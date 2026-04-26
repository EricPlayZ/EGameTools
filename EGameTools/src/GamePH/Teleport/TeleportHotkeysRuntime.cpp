#include <algorithm>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGT\GamePH\Teleport\TeleportRuntime.h>
#include <EGT\GamePH\Teleport\TeleportRuntime_Internal.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Teleport.h>

namespace EGT::GamePH::Teleport {
	static void UpdateTeleportPos() {
		if (IsTeleportationDisabled()) {
			if (!Menu::Teleport::teleportCoords.isDefault())
				Menu::Teleport::teleportCoords = {};
			return;
		}
		if (!Menu::Teleport::teleportCoords.isDefault())
			return;

		if (Menu::Camera::freeCam.GetValue()) {
			auto* freeCam = EGSDK::GamePH::FreeCamera::Get();
			if (freeCam)
				freeCam->GetPosition(&Menu::Teleport::teleportCoords);
		} else {
			auto* playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
			if (playerCharacter)
				Menu::Teleport::teleportCoords = playerCharacter->playerPos;
		}
	}

	static void UpdateHotkeys() {
		Menu::Teleport::teleportToSelectedLocation.SetChangesAreDisabled(Menu::Teleport::selectedTPLocation < 0 || Menu::Teleport::selectedTPLocation >= Menu::Teleport::savedTeleportLocations.size());
		Menu::Teleport::teleportToWaypoint.SetChangesAreDisabled(IsTeleportationDisabled() || !Menu::Teleport::waypointIsSet || !*Menu::Teleport::waypointIsSet);
		Menu::Teleport::teleportToCoords.SetChangesAreDisabled(IsTeleportationDisabled());

		if (Menu::Teleport::teleportToSelectedLocation.HasChanged()) {
			TeleportPlayerTo(Menu::Teleport::savedTeleportLocations[Menu::Teleport::selectedTPLocation].pos, Menu::Teleport::savedTeleportLocations[Menu::Teleport::selectedTPLocation].orientation);
			Menu::Teleport::teleportToSelectedLocation.SetPrevValue(Menu::Teleport::teleportToSelectedLocation.GetValue());
		}
		if (Menu::Teleport::teleportToWaypoint.HasChanged()) {
			Menu::Teleport::justTeleportedToWaypoint = TeleportPlayerTo(Menu::Teleport::waypointCoords);
			Menu::Teleport::teleportToWaypoint.SetPrevValue(Menu::Teleport::teleportToWaypoint.GetValue());
		}
		if (Menu::Teleport::teleportToCoords.HasChanged()) {
			TeleportPlayerTo(Menu::Teleport::teleportCoords);
			Menu::Teleport::teleportToCoords.SetPrevValue(Menu::Teleport::teleportToCoords.GetValue());
		}
	}

	void UpdateTeleportHotkeysRuntime() {
		Menu::Teleport::selectedTPLocation = std::clamp(Menu::Teleport::selectedTPLocation, -1, static_cast<int>(Menu::Teleport::savedTeleportLocations.size()) - 1);
		UpdateTeleportPos();
		UpdateHotkeys();
	}
}
