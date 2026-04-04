#include <EGSDK\Offsets.h>
#include <EGSDK\Utils\Sigscan.h>
#include <algorithm>
#include <initializer_list>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#include <Windows.h>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\Engine\CoPhysics.h>
#include <EGSDK\Engine\CGame.h>
#include <EGSDK\Engine\CVideoSettings.h>
#include <EGSDK\GamePH\CoPlayerRestrictions.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\InventoryMoney.h>
#include <EGSDK\GamePH\ItemDescWithContext.h>
#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LocalClientDI.h>
#include <EGSDK\GamePH\LogicalLevel.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\GamePH\PlayerHealthModule.h>
#include <EGSDK\GamePH\PlayerInfectionModule.h>
#include <EGSDK\GamePH\PlayerState.h>
#include <EGSDK\GamePH\SessionCooperativeDI.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGSDK\ClassHelpers.h>

namespace {
using namespace EGSDK;
using PatternT = Utils::SigScan::PatternType;

std::mutex patternGetterDiagMutex;
std::unordered_set<std::string> patternGetterFailuresLogged;

std::mutex lookupMissLogMutex;
std::unordered_set<std::string> patternLookupMissLogged;
std::unordered_set<std::string> offsetLookupMissLogged;

std::mutex namedPatternBackfillMutex;
std::unordered_set<uint32_t> namedPatternBackfilledForGameVer;

std::mutex memberOffsetBackfillMutex;
std::unordered_set<uint32_t> memberOffsetBackfillPassEngine;
std::unordered_set<uint32_t> memberOffsetBackfillPassGame;

template<typename Tier>
std::vector<size_t> TierTryOrder(const std::vector<Tier>& tiers, uint32_t gameVer) {
	std::vector<size_t> order;
	if (tiers.empty())
		return order;
	const size_t n = tiers.size();
	if (gameVer < tiers.front().sinceVersion) {
		for (size_t i = 0; i < n; ++i)
			order.push_back(i);
		return order;
	}
	if (gameVer >= tiers.back().sinceVersion) {
		for (size_t i = n; i > 0; --i)
			order.push_back(i - 1);
		return order;
	}
	size_t floorIdx = 0;
	for (size_t i = 0; i < n; ++i) {
		if (tiers[i].sinceVersion <= gameVer)
			floorIdx = i;
		else
			break;
	}
	for (size_t i = floorIdx; i < n; ++i)
		order.push_back(i);
	return order;
}

struct FixedOffsetTier {
	uint32_t sinceVersion;
	DWORD value;
};

struct FixedOffsetEntry {
	std::string key;
	std::vector<FixedOffsetTier> tiers;
};

struct VersionedPatternTier {
	uint32_t sinceVersion;
	Utils::SigScan::Pattern pattern;
};

struct NamedPatternEntry {
	std::string key;
	const char* moduleName;
	std::vector<VersionedPatternTier> tiers;
};

struct MemberDisplacementScan {
	const char* moduleOverride{};
	const char* signature{};
	PatternT patternType{ PatternT::Address };
};

// One tier: try each recipe in order. A recipe sums displacements from its scans (same module unless scan overrides).
struct MemberDisplacementTier {
	uint32_t sinceVersion{};
	const char* defaultModule{};
	std::vector<std::vector<MemberDisplacementScan>> recipes;
};

struct MemberOffsetEntry {
	std::string key;
	std::vector<MemberDisplacementTier> tiers;
};

using VersionedRegistryRow = std::variant<FixedOffsetEntry, NamedPatternEntry, MemberOffsetEntry>;

static MemberDisplacementScan MScan(const char* signature, PatternT patternType) {
	return { nullptr, signature, patternType };
}

static MemberDisplacementScan MScanMod(const char* moduleOverride, const char* signature, PatternT patternType) {
	return { moduleOverride, signature, patternType };
}

static MemberDisplacementTier MemTier(uint32_t sinceVersion, const char* defaultModule,
	std::initializer_list<std::initializer_list<MemberDisplacementScan>> recipeLists) {
	std::vector<std::vector<MemberDisplacementScan>> recipes;
	for (const auto& recipe : recipeLists)
		recipes.emplace_back(recipe.begin(), recipe.end());
	return MemberDisplacementTier{ sinceVersion, defaultModule, std::move(recipes) };
}

static MemberDisplacementTier Mem1(uint32_t sinceVersion, const char* defaultModule, const char* signature, PatternT patternType) {
	return MemTier(sinceVersion, defaultModule, { { MScan(signature, patternType) } });
}

// MSVC will not reliably deduce SortTiers<T>({ ... }); use explicit SortFixed / SortPat / SortMem.
template<typename Tier>
std::vector<Tier> SortTierList(std::initializer_list<Tier> il) {
	std::vector<Tier> v(il.begin(), il.end());
	std::sort(v.begin(), v.end(), [](const Tier& a, const Tier& b) {
		return a.sinceVersion < b.sinceVersion;
	});
	return v;
}

static std::vector<FixedOffsetTier> SortFixed(std::initializer_list<FixedOffsetTier> il) {
	return SortTierList(il);
}
static std::vector<VersionedPatternTier> SortPat(std::initializer_list<VersionedPatternTier> il) {
	return SortTierList(il);
}
static std::vector<MemberDisplacementTier> SortMem(std::initializer_list<MemberDisplacementTier> il) {
	return SortTierList(il);
}

static FixedOffsetEntry MakeFixed(const char* key, std::vector<FixedOffsetTier>&& tiers) {
	return { std::string(key), std::move(tiers) };
}

static NamedPatternEntry MakeNamed(const char* key, const char* module, std::vector<VersionedPatternTier>&& tiers) {
	return { std::string(key), module, std::move(tiers) };
}

template<typename ParentT, typename MemberT>
MemberOffsetEntry MakeMember(MemberT ParentT::* memberPointer, std::vector<MemberDisplacementTier>&& tiers) {
	return { ClassHelpers::GetOffsetNameFromClassMember(memberPointer), std::move(tiers) };
}

static DWORD ResolveMemberTierScan(const MemberDisplacementTier& tier) {
	for (const std::vector<MemberDisplacementScan>& recipe : tier.recipes) {
		DWORD sum = 0;
		bool ok = !recipe.empty();
		for (const MemberDisplacementScan& scan : recipe) {
			const char* mod = scan.moduleOverride ? scan.moduleOverride : tier.defaultModule;
			if (!mod || !scan.signature) {
				ok = false;
				break;
			}
			void* hit = Utils::SigScan::PatternScanner::FindPattern(mod, { scan.signature, scan.patternType });
			const DWORD part = hit ? static_cast<DWORD>(reinterpret_cast<uintptr_t>(hit)) : 0;
			if (!part) {
				ok = false;
				break;
			}
			sum += part;
		}
		if (ok && sum)
			return sum;
	}
	return 0;
}

template<class... Ts>
struct Overloaded : Ts... {
	using Ts::operator()...;
};
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

static void CollectBuildIdsFromRow(const VersionedRegistryRow& row, std::vector<uint32_t>& out) {
	std::visit(
		Overloaded{
			[&](const FixedOffsetEntry& e) {
				for (const FixedOffsetTier& t : e.tiers)
					out.push_back(t.sinceVersion);
			},
			[&](const NamedPatternEntry& e) {
				for (const VersionedPatternTier& t : e.tiers)
					out.push_back(t.sinceVersion);
			},
			[&](const MemberOffsetEntry& e) {
				for (const MemberDisplacementTier& t : e.tiers)
					out.push_back(t.sinceVersion);
			},
		},
		row);
}

// Module name tokens (not game build ids).
static constexpr const char kDllGame[] = "gamedll_ph_x64_rwdi.dll";
static constexpr const char kDllEngine[] = "engine_x64_rwdi.dll";

// ---------------------------------------------------------------------------
// One rows.push_back per key; tiers inline in SortFixed / SortPat / SortMem ({ ... }).
// Member tiers: Mem1(ver, module, sig, type) for a single scan; MemTier(ver, module, { {...}, {...} })
// for multiple recipes (first successful recipe wins; scans inside a recipe are summed).
// ---------------------------------------------------------------------------
static std::vector<VersionedRegistryRow> BuildVersionedGameRegistry() {
	using namespace Engine;
	using namespace GamePH;
	using GamePH::TimeWeather::CSystem;
	using GamePH::TimeWeather::ISubsystem;
	using PT = PatternT;

	constexpr const char kHealthLea[] = "49 8D 4E [?? E8 ?? ?? ?? ?? 48 8B 45 ?? 4C 8B AC 24";
	constexpr const char kHealthMaxInner[] = "F3 0F 10 41 [?? C3 CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B 49 ?? 4C 8D 44 24";

	std::vector<VersionedRegistryRow> rows;
	rows.reserve(64);

	rows.push_back(MakeFixed("OnPostUpdate", SortFixed({
		{ 11200u, 0x378u },
		{ 12001u, 0x3A8u },
	})));

	rows.push_back(MakeNamed("LoadPlayerVars", kDllGame, SortPat({
		{ 11200u, { "40 55 53 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 33 FF", PT::Address } },
		{ 12001u, { "48 89 4C 24 ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 8C 24", PT::Address } },
	})));
	rows.push_back(MakeNamed("PlayerState", kDllGame, SortPat({
		{ 11200u, { "48 8B 3D [?? ?? ?? ?? 4C 8B EA", PT::RelativePointer } },
		{ 12001u, { "48 8B 35 [?? ?? ?? ?? 4C 8B F2 48 8B F9", PT::RelativePointer } },
	})));
	rows.push_back(MakeNamed("SaveGameCRCBoolCheck", kDllGame, SortPat({
		{ 11200u, { "FF 50 ?? [40 22 FB 0F 85 ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 48 8D 1D", PT::Address } },
		{ 12001u, { "FF 50 ?? [40 22 DF 0F 85 ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 48 8D 3D", PT::Address } },
	})));
	rows.push_back(MakeNamed("IsNotOutOfMapBounds", kDllGame, SortPat({
		{ 11200u, { "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4C 8B C2", PT::Address } },
		{ 12001u, { "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4C 8B F9 48 85 D2", PT::Address } },
	})));
	rows.push_back(MakeNamed("IsNotOutOfMissionBounds", kDllGame, SortPat({
		{ 11200u, { "48 89 5C 24 ?? 57 48 83 EC ?? 4C 8B C2 48 8B F9", PT::Address } },
		{ 12001u, { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 85 D2 74 ?? 48 8D 8A", PT::Address } },
	})));
	rows.push_back(MakeNamed("PlaySoundEvent", kDllGame, SortPat({
		{ 11200u, { "4C 8B DC 49 89 5B ?? 49 89 73 ?? 57 48 81 EC ?? ?? ?? ?? 4C 8B 4C 24 ?? 48 8B F9 4D 8B D0 66 C7 84 24 ?? ?? ?? ?? ?? ?? 49 8B C1 66 C7 84 24", PT::Address } },
		{ 12001u, { "4C 8B DC 49 89 5B ?? 49 89 73 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 44 24 ?? 48 8B F9 48 8B DA", PT::Address } },
	})));
	rows.push_back(MakeNamed("GetPlayerRestrictionsFlags", kDllGame, SortPat({
		{ 11200u, { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B F9 48 8B DA 48 8B CA E8 ?? ?? ?? ?? 48 8B 4F", PT::Address } },
		{ 12001u, { "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B D9 48 8B FA 48 8B CA E8 ?? ?? ?? ?? 48 8B 4B", PT::Address } },
	})));
	rows.push_back(MakeNamed("EnablePlayerRestrictionsSubFunc", kDllGame, SortPat({
		{ 11200u, { "40 53 48 83 EC ?? 48 8B D9 48 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B CB 48 83 C4 ?? 5B E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 4C 24", PT::Address } },
		{ 12001u, { "40 53 48 83 EC ?? 48 8B D9 48 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B CB 48 83 C4 ?? 5B E9 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC 40 57", PT::Address } },
	})));
	rows.push_back(MakeNamed("DisablePlayerRestrictionsSubFunc", kDllGame, SortPat({
		{ 11200u, { "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 0F B7 81", PT::Address } },
		{ 12001u, { "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 0F B6 81", PT::Address } },
	})));
	rows.push_back(MakeNamed("HandlePlayerRestrictions", kDllGame, SortPat({
		{ 11200u, { "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 6C 24 ?? 0F B6 A9", PT::Address } },
		{ 12001u, { "40 57 48 83 EC ?? 48 89 5C 24 ?? 48 8B F9 48 89 74 24 ?? 0F B6 B1", PT::Address } },
	})));
	rows.push_back(MakeNamed("GetCoPhysics", kDllGame, SortPat({
		{ 12001u, { "48 83 EC ?? 48 8B C1 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 FF 50 ?? EB ?? 48 05 ?? ?? ?? ?? 48 8B 10 48 85 D2 74 ?? 48 8B 0D ?? ?? ?? ?? 48 83 C4 ?? 48 FF 25 ?? ?? ?? ?? 33 C0 48 83 C4 ?? C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53", PT::Address } },
	})));

	rows.push_back(MakeMember(&CGame::pGameDI_PH, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 86 [?? ?? ?? ?? 48 8B 48 ?? 48 85 C9 74 ?? 48 8B 01", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CGame::pCVideoSettings, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 80 [?? ?? ?? ?? 8B 40 ?? C3 CC 48 8B 05", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CGame::pCLevel, SortMem({
		Mem1(12001u, kDllEngine, "48 89 83 [?? ?? ?? ?? B0 ?? 48 83 C4 ?? 5B C3 CC CC CC CC CC CC CC 4C 8B DC", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CVideoSettings::extraFOV, SortMem({
		Mem1(12001u, kDllEngine, "F3 0F 10 41 [?? C3 CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CBulletPhysicsCharacter::playerPos, SortMem({
		Mem1(12001u, kDllEngine, "F2 0F 11 81 [?? ?? ?? ?? 8B 42 ?? B2", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CBulletPhysicsCharacter::playerDownwardVelocity, SortMem({
		Mem1(12001u, kDllEngine, "44 0F 2F 9F [?? ?? ?? ?? F3 0F 10 0D", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CoPhysics::pCBulletPhysicsCharacter, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 49 [?? 48 85 C9 74 ?? 48 8B 01 48 FF 60 ?? 33 C0 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC ?? 48 8B 01", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CSystem::blendTime, SortMem({
		Mem1(12001u, kDllEngine, "F3 0F 10 7F [?? 80 7F", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CSystem::blendTime2, SortMem({
		Mem1(12001u, kDllEngine, "F3 0F 10 7F [?? EB", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CSystem::currentSubSystem, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 41 [?? 48 85 C0 75 ?? B8 ?? ?? ?? ?? C3", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CSystem::nextSubSystem, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 41 [?? 48 85 C0 75 ?? 48 8B 41 ?? 48 85 C0 75 ?? B8", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&CSystem::lastSubSystem, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 99 [?? ?? ?? ?? 48 8B F9 48 85 DB 74 ?? 48 8B 49", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&ISubsystem::nextTime, SortMem({
		Mem1(12001u, kDllEngine, "8B 53 [?? 0F 44 E9", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&FreeCamera::pCoBaseCameraProxy, SortMem({
		Mem1(12001u, kDllEngine, "48 8B 49 [?? 48 85 C9 74 ?? 80 79", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&FreeCamera::pCBaseCamera, SortMem({
		Mem1(12001u, kDllGame, "48 8B 49 [?? F3 0F 11 4C 24 ?? F3 0F 10 89", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&FreeCamera::enableSpeedMultiplier1, SortMem({
		Mem1(12001u, kDllGame, "80 7E [?? ?? F3 44 0F 10 B6", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&FreeCamera::enableSpeedMultiplier2, SortMem({
		Mem1(12001u, kDllGame, "80 7E [?? ?? 74 ?? F3 44 0F 59 B6", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&FreeCamera::speedMultiplier, SortMem({
		Mem1(12001u, kDllGame, "F3 44 0F 59 B6 [?? ?? ?? ?? 80 7E", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&GameDI_PH::blockPauseGameOnPlayerAfk, SortMem({
		Mem1(12001u, kDllGame, "80 BB [?? ?? ?? ?? ?? 74 ?? 40 B7 ?? EB ?? 40 32 FF 48 8B 03", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&GameDI_PH::pSessionCooperativeDI, SortMem({
		Mem1(12001u, kDllGame, "48 83 B9 [?? ?? ?? ?? ?? 0F 95 C0 C3 CC CC CC CC 48 83 EC ?? E8", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&SessionCooperativeDI::pLocalClientDI, SortMem({
		Mem1(12001u, kDllGame, "48 8B 88 [?? ?? ?? ?? 48 85 C9 74 ?? 48 83 B9 ?? ?? ?? ?? ?? 74 ?? 48 8B 81 ?? ?? ?? ?? 48 83 C4", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&SessionCooperativeDI::pLogicalLevel, SortMem({
		Mem1(12001u, kDllGame, "48 8B 81 [?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 0F B6 81 ?? ?? ?? ?? 84 C0", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&LogicalLevel::pLogicalPlayer, SortMem({
		Mem1(12001u, kDllGame, "48 8B 8B [?? ?? ?? ?? 48 8B 74 24 ?? 48 85 C9 74 ?? 48 8B 01", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&LocalClientDI::pPlayerDI_PH, SortMem({
		Mem1(12001u, kDllGame, "48 8B 81 [?? ?? ?? ?? 48 83 C4 ?? 5B C3 33 C0", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerDI_PH::pInventoryContainerDI, SortMem({
		Mem1(12001u, kDllGame, "48 8B 8E [?? ?? ?? ?? 48 85 C9 74 ?? BA", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerDI_PH::nextPlayerOrientation, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 81 [?? ?? ?? ?? F3 0F 10 89 ?? ?? ?? ?? 48 89 BC 24", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerDI_PH::restrictionsEnabled, SortMem({
		Mem1(12001u, kDllGame, "8B A9 [?? ?? ?? ?? EB ?? FE C8 0F B6 E8 4C 8B 89", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerDI_PH::enableTPPModel1, SortMem({
		Mem1(12001u, kDllGame, "80 B8 [?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B 01 44 0F 29 84 24", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerDI_PH::enableTPPModel2, SortMem({
		Mem1(12001u, kDllGame, "41 3A 95 [?? ?? ?? ??", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&CoPlayerRestrictions::flags, SortMem({
		Mem1(12001u, kDllGame, "48 8D 97 [?? ?? ?? ?? 48 8B 08 48 89 0A", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerInfectionModule::pPlayerDI_PH, SortMem({
		Mem1(12001u, kDllGame, "48 8B 49 [?? 48 83 C1 ?? 48 8B 01 48 FF A0 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 8B 49", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&PlayerInfectionModule::maxImmunity, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 5E 40 [?? 48 8B C3", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&PlayerInfectionModule::immunity, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 48 [?? F3 0F 5E 48 ?? F3 0F 5C C1", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&PlayerInfectionModule::nightrunnerTimer, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 B1 [?? ?? ?? ?? 48 8B D9 0F 2F F0 76 ?? 48 8B 01 F3 0F 58 B1 ?? ?? ?? ?? FF 90 ?? ?? ?? ?? 0F 2F F0 77 ?? F3 0F 10 05 ?? ?? ?? ?? 0F 2E 83 ?? ?? ?? ?? 75 ?? B0", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerState::playerVariables, SortMem({
		Mem1(12001u, kDllGame, "48 8B 80 [?? ?? ?? ?? C3 CC 80 3D", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&DayNightCycle::time1, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 70 [?? 0F 2E B1", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&DayNightCycle::time2, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 4B [?? 8B 6B", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&DayNightCycle::time3, SortMem({
		Mem1(12001u, kDllGame, "48 8D 4B [?? 0F 2F 0D", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&InventoryMoney::oldWorldMoney, SortMem({
		Mem1(12001u, kDllGame, "48 8D 59 [?? 48 8B F9 85 D2", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&ItemDescWithContext::weaponDurability, SortMem({
		Mem1(12001u, kDllGame, "F3 0F 10 81 [?? ?? ?? ?? C3 CC CC CC CC CC CC CC 85 D2", PT::MemberDisplacement32),
	})));
	rows.push_back(MakeMember(&PlayerHealthModule::pPlayerDI_PH, SortMem({
		Mem1(12001u, kDllGame, "48 8B 48 [?? 48 8B 11 FF 92 ?? ?? ?? ?? 48 8B 17", PT::MemberDisplacement8),
	})));
	rows.push_back(MakeMember(&PlayerHealthModule::health, SortMem({
		MemTier(12001u, kDllGame, {
			{
				MScan("41 0F 11 46 [?? 0F 10 45 ?? 48 8B 6C 24", PT::MemberDisplacement8),
				MScan(kHealthMaxInner, PT::MemberDisplacement8),
				MScan(kHealthLea, PT::MemberDisplacement8),
			},
		}),
	})));
	rows.push_back(MakeMember(&PlayerHealthModule::maxHealth, SortMem({
		MemTier(12001u, kDllGame, {
			{
				MScan("41 0F 11 46 [?? 48 83 C4 ?? 41 5E C3 CC CC CC CC CC CC CC 48 8B C4", PT::MemberDisplacement8),
				MScan(kHealthMaxInner, PT::MemberDisplacement8),
				MScan(kHealthLea, PT::MemberDisplacement8),
			},
		}),
	})));

	return rows;
}

static const std::vector<VersionedRegistryRow>& GetVersionedRegistry() {
	static const std::vector<VersionedRegistryRow> kRegistry = BuildVersionedGameRegistry();
	return kRegistry;
}

// Fills named-pattern keys that are still missing (e.g. gamedll was not loaded during InitializeOffsetsAndPatterns).
static void FillMissingNamedPatterns(uint32_t gameVer, std::unordered_map<std::string, Utils::SigScan::Pattern>& patternMap) {
	for (const VersionedRegistryRow& row : GetVersionedRegistry()) {
		std::visit(
			Overloaded{
				[](const FixedOffsetEntry&) {},
				[&](const NamedPatternEntry& e) {
					if (patternMap.find(e.key) != patternMap.end())
						return;
					if (!GetModuleHandleA(e.moduleName))
						return;
					const std::vector<size_t> order = TierTryOrder(e.tiers, gameVer);
					Utils::SigScan::Pattern resolved{};
					for (const size_t idx : order) {
						const VersionedPatternTier& t = e.tiers[idx];
						if (!t.pattern.pattern)
							continue;
						void* hit = Utils::SigScan::PatternScanner::FindPattern(e.moduleName, t.pattern);
						if (hit) {
							resolved = t.pattern;
							break;
						}
					}
					if (resolved.pattern)
						patternMap[e.key] = resolved;
				},
				[](const MemberOffsetEntry&) {},
			},
			row);
	}
}

// One backfill pass per gameVer once gamedll is present — avoids scanning the registry on every GetPattern miss.
static void MaybeBackfillNamedPatterns(uint32_t gameVer, std::unordered_map<std::string, Utils::SigScan::Pattern>& patternMap) {
	if (!GetModuleHandleA("gamedll_ph_x64_rwdi.dll"))
		return;
	std::lock_guard<std::mutex> lock(namedPatternBackfillMutex);
	if (!namedPatternBackfilledForGameVer.insert(gameVer).second)
		return;
	FillMissingNamedPatterns(gameVer, patternMap);
}

static void FillMissingMemberOffsets(uint32_t gameVer, std::unordered_map<std::string, DWORD>& offsetMap) {
	for (const VersionedRegistryRow& row : GetVersionedRegistry()) {
		std::visit(
			Overloaded{
				[](const FixedOffsetEntry&) {},
				[](const NamedPatternEntry&) {},
				[&](const MemberOffsetEntry& e) {
					if (offsetMap.find(e.key) != offsetMap.end())
						return;
					const std::vector<size_t> order = TierTryOrder(e.tiers, gameVer);
					DWORD offsetValue = 0;
					for (const size_t idx : order) {
						const MemberDisplacementTier& t = e.tiers[idx];
						if (!t.defaultModule || t.recipes.empty())
							continue;
						if (!GetModuleHandleA(t.defaultModule))
							continue;
						offsetValue = ResolveMemberTierScan(t);
						if (offsetValue)
							break;
					}
					if (offsetValue)
						offsetMap[e.key] = offsetValue;
				},
			},
			row);
	}
}

// One full member scan per gameVer when engine loads, and again when gamedll loads (many fields need gamedll).
static void MaybeBackfillMemberOffsets(uint32_t gameVer, std::unordered_map<std::string, DWORD>& offsetMap) {
	std::lock_guard<std::mutex> lock(memberOffsetBackfillMutex);
	if (GetModuleHandleA(kDllEngine) && memberOffsetBackfillPassEngine.insert(gameVer).second)
		FillMissingMemberOffsets(gameVer, offsetMap);
	if (GetModuleHandleA(kDllGame) && memberOffsetBackfillPassGame.insert(gameVer).second)
		FillMissingMemberOffsets(gameVer, offsetMap);
}

static void ApplyVersionedGameRegistry(uint32_t gameVer, std::unordered_map<std::string, DWORD>& offsetMap,
	std::unordered_map<std::string, Utils::SigScan::Pattern>& patternMap) {
	const std::vector<VersionedRegistryRow>& kRegistry = GetVersionedRegistry();

#ifdef _DEBUG
	static bool loggedBuilds = false;
	if (!loggedBuilds) {
		loggedBuilds = true;
		std::vector<uint32_t> ids;
		for (const VersionedRegistryRow& row : kRegistry)
			CollectBuildIdsFromRow(row, ids);
		std::sort(ids.begin(), ids.end());
		ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
		std::ostringstream oss;
		for (size_t i = 0; i < ids.size(); ++i) {
			if (i)
				oss << ", ";
			oss << ids[i];
		}
		SPDLOG_DEBUG("[OffsetManager] Registry declares tier build ids: {}", oss.str());
	}
#endif

	for (const VersionedRegistryRow& row : kRegistry) {
		std::visit(
			Overloaded{
				[&](const FixedOffsetEntry& e) {
					const std::vector<size_t> order = TierTryOrder(e.tiers, gameVer);
					if (!order.empty())
						offsetMap[e.key] = e.tiers[order.front()].value;
				},
				[&](const NamedPatternEntry& e) {
					const std::vector<size_t> order = TierTryOrder(e.tiers, gameVer);
					Utils::SigScan::Pattern resolved{};
					for (const size_t idx : order) {
						const VersionedPatternTier& t = e.tiers[idx];
						if (!t.pattern.pattern)
							continue;
						void* hit = Utils::SigScan::PatternScanner::FindPattern(e.moduleName, t.pattern);
						if (hit) {
							resolved = t.pattern;
							break;
						}
					}
					if (resolved.pattern)
						patternMap[e.key] = resolved;
					else
						SPDLOG_ERROR("[OffsetManager] Named pattern miss: \"{}\" (game v{}, {} tier attempt(s))", e.key, gameVer, order.size());
				},
				[&](const MemberOffsetEntry& e) {
					const std::vector<size_t> order = TierTryOrder(e.tiers, gameVer);
					DWORD offsetValue = 0;
					for (const size_t idx : order) {
						const MemberDisplacementTier& t = e.tiers[idx];
						if (!t.defaultModule || t.recipes.empty())
							continue;
						offsetValue = ResolveMemberTierScan(t);
						if (offsetValue)
							break;
					}
					if (offsetValue)
						offsetMap[e.key] = offsetValue;
					else
						SPDLOG_ERROR("[OffsetManager] Member offset pattern miss: \"{}\" (game v{}, {} tier attempt(s))", e.key, gameVer, order.size());
				},
			},
			row);
	}
}

} // namespace

namespace EGSDK {

	bool OffsetManager::initialized = false;

	void OffsetManager::InitializeOffsetsAndPatterns() {
		if (initialized)
			return;

		if (!GetModuleHandleA("gamedll_ph_x64_rwdi.dll"))
			SPDLOG_WARN("[OffsetManager] gamedll_ph_x64_rwdi.dll is not loaded yet; gamedll patterns may miss until it loads.");

		const uint32_t gameVer = Core::gameVer;
		std::unordered_map<std::string, DWORD>& offsetMap = GetOffsetsMap()[gameVer];
		std::unordered_map<std::string, Utils::SigScan::Pattern>& patternMap = GetPatternsMap()[gameVer];

		ApplyVersionedGameRegistry(gameVer, offsetMap, patternMap);

		initialized = true;
	}
	DWORD OffsetManager::GetOffset(const std::string& offsetName) {
		if (!initialized)
			return 0;

		const uint32_t gv = Core::gameVer;
		auto& offsets = GetOffsetsMap();
		std::unordered_map<std::string, DWORD>& sub = offsets[gv];

		const auto itHit = sub.find(offsetName);
		if (itHit != sub.end())
			return itHit->second;

		MaybeBackfillMemberOffsets(gv, sub);

		const auto itAfter = sub.find(offsetName);
		if (itAfter != sub.end())
			return itAfter->second;

		std::string dedup = std::to_string(gv);
		dedup.push_back('\x1f');
		dedup += offsetName;
		{
			std::lock_guard<std::mutex> lock(lookupMissLogMutex);
			if (!offsetLookupMissLogged.insert(dedup).second)
				return 0;
		}
		SPDLOG_ERROR("Offset not found for key: \"{}\" in version: {}", offsetName, gv);
		return 0;
	}
	Utils::SigScan::Pattern OffsetManager::GetPattern(const std::string& patternName) {
		if (!initialized)
			return {};

		const uint32_t gv = Core::gameVer;
		auto& patterns = GetPatternsMap();
		std::unordered_map<std::string, Utils::SigScan::Pattern>& sub = patterns[gv];

		const auto itHit = sub.find(patternName);
		if (itHit != sub.end())
			return itHit->second;

		MaybeBackfillNamedPatterns(gv, sub);

		const auto itAfter = sub.find(patternName);
		if (itAfter != sub.end())
			return itAfter->second;

		std::string dedup = std::to_string(gv);
		dedup.push_back('\x1f');
		dedup += patternName;
		{
			std::lock_guard<std::mutex> lock(lookupMissLogMutex);
			if (!patternLookupMissLogged.insert(dedup).second)
				return {};
		}
		SPDLOG_ERROR("Pattern not found for key: \"{}\" in version: {}", patternName, gv);
		return {};
	}

	void OffsetManager::AddOffsets(DWORD gameVer, const std::unordered_map<std::string, DWORD>& offsets) {
		GetOffsetsMap()[gameVer] = offsets;
	}
	void OffsetManager::AddPatterns(DWORD gameVer, const std::unordered_map<std::string, Utils::SigScan::Pattern>& patterns) {
		GetPatternsMap()[gameVer] = patterns;
	}

	std::unordered_map<DWORD, std::unordered_map<std::string, DWORD>>& OffsetManager::GetOffsetsMap() {
		static std::unordered_map<DWORD, std::unordered_map<std::string, DWORD>> offsetsMap;
		return offsetsMap;
	}
	std::unordered_map<DWORD, std::unordered_map<std::string, Utils::SigScan::Pattern>>& OffsetManager::GetPatternsMap() {
		static std::unordered_map<DWORD, std::unordered_map<std::string, Utils::SigScan::Pattern>> patternsMap;
		return patternsMap;
	}

	void OffsetManager::LogPatternGetterFailure(const char* getterSymbol, const char* moduleName, const char* reason) {
		std::ostringstream oss;
		oss << "Get_" << getterSymbol << ": " << reason << " [module=\"" << moduleName << "\"]";
		const std::string full = oss.str();

		{
			std::lock_guard<std::mutex> lock(patternGetterDiagMutex);
			const std::string dedupKey = std::string(getterSymbol) + '\x1f' + reason;
			if (!patternGetterFailuresLogged.insert(dedupKey).second)
				return;
		}
		SPDLOG_WARN("[OffsetManager] {}", full);
	}
}
