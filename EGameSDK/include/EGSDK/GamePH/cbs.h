#pragma once
#include <stdint.h>
#include <EGSDK\Exports.h>
#include <EGSDK\mtx34.h>

namespace EGSDK::GamePH {
	class EGameSDK_API cbs {
		public:
			static void* GetILevel(uint32_t worldIndex);
			static void* CreateEntityFromPrefab(const char* prefabPathUtf8Z, void* iLevel, const mtx34* rootTransform, const char* presetNameUtf8Z, std::uint64_t* outEntityCPointer);
		};
}
