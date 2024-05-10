#pragma once
#include "..\buffer.h"
#include "ItemDescWithContext.h"

namespace GamePH {
	class InventoryItem {
	public:
		ItemDescWithContext* GetItemDescCtx();
	};
}