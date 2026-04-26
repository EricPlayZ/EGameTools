#include <cstdint>
#include <EGSDK\GamePH\cbs.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::GamePH {
	void* cbs::GetILevel(std::uint32_t worldIndex) {
		return Utils::Memory::SafeCallFunction<void*>("engine_x64_rwdi.dll", "?GetILevel@cbs@@YAPEAVILevel@@UWorldIndex@1@@Z", nullptr, worldIndex);
	}

	void* cbs::CreateEntityFromPrefab(const char* prefabPathUtf8Z, void* iLevel, const mtx34* rootTransform, const char* presetNameUtf8Z, std::uint64_t* outEntityCPointer) {
		alignas(8) std::uint8_t replicationOptions[64]{};
		alignas(8) std::uint64_t entityBitsLocal{};

		void* const hiddenReturn = outEntityCPointer ? static_cast<void*>(outEntityCPointer) : static_cast<void*>(&entityBitsLocal);
		const std::uint64_t pathBits = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(prefabPathUtf8Z));
		const std::uint64_t presetBits = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(presetNameUtf8Z));

		return Utils::Memory::SafeCallFunction<void*>("engine_x64_rwdi.dll", "?CreateEntityFromPrefab@cbs@@YA?AV?$CPointer@VCEntity@cbs@@@1@V?$string_const@D@ttl@@PEAVILevel@@AEBVmtx34@@0VCreateObjectOptions@Replication@@_N4@Z", nullptr, hiddenReturn, pathBits, iLevel, rootTransform, presetBits, replicationOptions, static_cast<unsigned char>(1), static_cast<unsigned char>(1));
	}
}
