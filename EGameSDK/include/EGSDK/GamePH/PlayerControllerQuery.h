#pragma once
#include <EGSDK\Exports.h>

namespace EGSDK::GamePH {
	class EGameSDK_API PlayerControllerQuery {
	public:
		// playerDI_PH must be non-null. rttiLeaf e.g. "PlayerHealthModule".
		// Pass a static const void* in cachedVtable: first RTTI hit writes the vtable; later calls prefer a vtable walk.
		static void* TryFindControllerByRtti(void* playerDI_PH, const char* rttiLeaf, const void** cachedVtable);
	};
}
