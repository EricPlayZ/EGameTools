#include <pch.h>
#include "..\offsets.h"
#include "InventoryItem.h"
#include "ItemDescWithContext.h"

namespace GamePH {
	ItemDescWithContext* InventoryItem::GetItemDescCtx() {
		__try {
			ItemDescWithContext* ptr = reinterpret_cast<ItemDescWithContext*>(reinterpret_cast<DWORD64>(this) + 0x40);
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			if (*reinterpret_cast<DWORD64**>(ptr) != Offsets::GetVT_ItemDescWithContext())
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}