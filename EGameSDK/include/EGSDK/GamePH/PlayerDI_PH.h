#pragma once
#include <stdint.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\vec2.h>
#include <EGSDK\Engine\IControlObject.h>
#include <EGSDK\Engine\IModelObject.h>
#include <EGSDK\GamePH\InventoryItem.h>
#include <EGSDK\GamePH\InventoryContainerDI.h>

namespace EGSDK::Engine {
	class CoPhysicsProperty;
}

namespace EGSDK::GamePH {
	class EGameSDK_API PlayerDI_PH : public Engine::IControlObject, public IModelObject {
	public:
		union {
			DynamicField(PlayerDI_PH, Engine::CoPhysicsProperty*, pCoPhysicsProperty);
			DynamicField(PlayerDI_PH, InventoryContainerDI*, pInventoryContainerDI);
			DynamicField(PlayerDI_PH, vec2, nextPlayerOrientation);
			DynamicField(PlayerDI_PH, bool, restrictionsEnabled);
			DynamicField(PlayerDI_PH, bool, enableTPPModel1);
			DynamicField(PlayerDI_PH, bool, enableTPPModel2);
		};

		static bool areRestrictionsEnabledByGame;

		InventoryItem* GetCurrentWeapon(uint32_t indexMaybe);
		InventoryContainerDI* GetInventoryContainer();

		bool EnablePlayerRestrictions(uint64_t* flags);
		bool DisablePlayerRestrictions(uint64_t* flags);
		bool HandlePlayerRestrictions();

		static PlayerDI_PH* Get();
	};
}