#include <EGSDK\GamePH\PlayerControllerQuery.h>
#include <EGSDK\Offsets.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\RTTI.h>
#include <cstdint>
#include <string>
#include <string_view>

namespace EGSDK::GamePH {
	constexpr int controllerScanHardMax = 4096;
	constexpr int controllerScanFallback = 512;
	constexpr uint32_t bytesAfterListPointerToCapacityMember = sizeof(void*);

	uint32_t listMemberOffsetFromGetPlayerController = 0;

	bool RttiNameIs(const std::string& name, std::string_view leaf) {
		if (name.empty() || name == "bad_read_class" || name == "bad_read_vtable")
			return false;
		if (name == leaf)
			return true;
		const std::string suffix = std::string("::") + std::string(leaf);
		return name.size() >= suffix.size()
			&& name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
	}

	void TryParseListOffsetFromPrologue(void* functionAddress) {
		if (!functionAddress || Utils::Memory::IsBadReadPtr(functionAddress))
			return;
		const uint8_t* code = reinterpret_cast<const uint8_t*>(functionAddress);
		// mov rax, [rcx + imm32]  —  offset of controller list pointer on PlayerDI
		if (code[0] == 0x48 && code[1] == 0x8B && code[2] == 0x81) {
			const int32_t listPointerMemberOffset = *reinterpret_cast<const int32_t*>(code + 3);
			if (listPointerMemberOffset > 0 && listPointerMemberOffset < 0x10000)
				listMemberOffsetFromGetPlayerController = static_cast<uint32_t>(listPointerMemberOffset);
		}
	}

	// Resolves pattern address once per call (OffsetManager caches); decodes list/capacity layout from the function bytes.
	bool EnsureControllerTableLayoutFromPattern() {
		void* functionAddress = OffsetManager::Get_GetPlayerController();
		if (!functionAddress || Utils::Memory::IsBadReadPtr(functionAddress))
			return false;
		if (listMemberOffsetFromGetPlayerController == 0)
			TryParseListOffsetFromPrologue(functionAddress);
		return true;
	}

	void* GetPlayerController(void* playerDI, int slotIndex) {
		if (!playerDI)
			return nullptr;
		return Utils::Memory::SafeCallFunctionOffset<void*>(OffsetManager::Get_GetPlayerController, nullptr, playerDI, slotIndex);
	}

	int GetControllerSlotUpperBound(void* playerDI) {
		if (!playerDI)
			return controllerScanFallback;
		EnsureControllerTableLayoutFromPattern();
		if (listMemberOffsetFromGetPlayerController == 0)
			return controllerScanFallback;

		const uint32_t capacityMemberOffset = listMemberOffsetFromGetPlayerController + bytesAfterListPointerToCapacityMember;
		const auto* base = reinterpret_cast<const uint8_t*>(playerDI);
		if (Utils::Memory::IsBadReadPtr(const_cast<uint8_t*>(base + capacityMemberOffset), sizeof(uint32_t)))
			return controllerScanFallback;
		const uint32_t capacity = *reinterpret_cast<const uint32_t*>(base + capacityMemberOffset);
		if (capacity == 0 || capacity > static_cast<uint32_t>(controllerScanHardMax))
			return controllerScanFallback;
		return static_cast<int>(capacity);
	}

	void* FindByVtable(void* playerDI, const void* expectedVtable) {
		if (!expectedVtable || !playerDI)
			return nullptr;
		const int limit = GetControllerSlotUpperBound(playerDI);
		for (int slotIndex = 0; slotIndex < limit; ++slotIndex) {
			void* candidate = GetPlayerController(playerDI, slotIndex);
			if (!candidate || Utils::Memory::IsBadReadPtr(candidate))
				continue;
			void* vtable = *reinterpret_cast<void* const*>(candidate);
			if (Utils::Memory::IsBadReadPtr(vtable))
				continue;
			if (vtable == expectedVtable)
				return candidate;
		}
		return nullptr;
	}

	void* PlayerControllerQuery::TryFindControllerByRtti(void* playerDI_PH, const char* rttiLeaf, const void** cachedVtable) {
		if (!playerDI_PH || !rttiLeaf)
			return nullptr;
		if (!EnsureControllerTableLayoutFromPattern())
			return nullptr;

		if (cachedVtable && *cachedVtable) {
			if (void* controllerSlot = FindByVtable(playerDI_PH, *cachedVtable))
				return controllerSlot;
		}

		const int limit = GetControllerSlotUpperBound(playerDI_PH);
		for (int slotIndex = 0; slotIndex < limit; ++slotIndex) {
			void* candidate = GetPlayerController(playerDI_PH, slotIndex);
			if (!candidate || Utils::Memory::IsBadReadPtr(candidate))
				continue;

			void* vtable = *reinterpret_cast<void* const*>(candidate);
			if (!vtable || Utils::Memory::IsBadReadPtr(vtable))
				continue;

			const std::string name = Utils::RTTI::GetVTableName(candidate);
			if (!RttiNameIs(name, rttiLeaf))
				continue;

			if (cachedVtable)
				*cachedVtable = vtable;
			return candidate;
		}

		return nullptr;
	}
}
