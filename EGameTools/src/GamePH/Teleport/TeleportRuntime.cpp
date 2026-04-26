#include <format>
#include <spdlog\spdlog.h>
#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGT\GamePH\Teleport\TeleportRuntime.h>
#include <EGT\GamePH\Teleport\TeleportRuntime_Internal.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Player.h>
#include <EGT\Menu\Teleport.h>

namespace EGT::GamePH::Teleport {
	std::string GetFormattedPosition(const vec3* position) {
		if (!position || position->isDefault())
			return "X: 0.00, Y: 0.00, Z: 0.00";
		return std::format("X: {:.2f}, Y: {:.2f}, Z: {:.2f}", position->X, position->Y, position->Z);
	}

	bool IsTeleportationDisabled() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return true;
		if (!Menu::Camera::freeCam.GetValue() && !EGSDK::Engine::CBulletPhysicsCharacter::Get())
			return true;
		else if (Menu::Camera::freeCam.GetValue() && !EGSDK::GamePH::FreeCamera::Get())
			return true;

		return false;
	}

	void SyncPlayerCoordsToTPCoords() {
		if (IsTeleportationDisabled())
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

	bool TeleportPlayerTo(const vec3& pos, const vec2& orientation) {
		if (IsTeleportationDisabled() || pos.isDefault()) {
			if (pos.isDefault())
				SPDLOG_ERROR("Teleport position was default, couldn't teleport player");
			return false;
		}

		if (Menu::Camera::freeCam.GetValue()) {
			auto* freeCam = EGSDK::GamePH::FreeCamera::Get();
			if (!freeCam)
				return false;
			freeCam->SetPosition(&pos);
		} else {
			auto* playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
			if (!playerCharacter)
				return false;
			auto* playerDI_PH = EGSDK::GamePH::PlayerDI_PH::Get();
			if (!playerDI_PH && !orientation.isDefault())
				SPDLOG_ERROR("PlayerDI_PH was null, won't be able to set player teleport orientation");

			if (Menu::Player::freezePlayer.GetValue())
				playerCharacter->posBeforeFreeze = pos;

			playerCharacter->MoveCharacter(pos);
			if (playerDI_PH && !orientation.isDefault())
				playerDI_PH->nextPlayerOrientation->X = orientation.X;
		}

		return true;
	}

	void UpdateRuntimeState() {
		UpdateTeleportHotkeysRuntime();
	}
}
