#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Player\PlayerRuntime_Internal.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Player.h>

namespace EGT::GamePH::Player {
	void UpdatePlayerPositionRuntime() {
		auto* playerCharacter = EGSDK::Engine::CBulletPhysicsCharacter::Get();
		if (!playerCharacter)
			return;
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel)
			return;

		if ((Menu::Player::freezePlayer.GetValue() || (Menu::Camera::freeCam.GetValue() && !Menu::Camera::teleportPlayerToCamera.GetValue())) && !iLevel->IsTimerFrozen()) {
			playerCharacter->FreezeCharacter();
			return;
		}

		EGSDK::Engine::CBulletPhysicsCharacter::posBeforeFreeze = playerCharacter->playerPos;
		if (iLevel->IsTimerFrozen() || !Menu::Camera::freeCam.GetValue() || !Menu::Camera::teleportPlayerToCamera.GetValue())
			return;

		auto* freeCam = EGSDK::GamePH::FreeCamera::Get();
		if (!freeCam)
			return;

		vec3 camPos{};
		freeCam->GetPosition(&camPos);
		if (!camPos.isDefault())
			playerCharacter->MoveCharacter(camPos);
	}
}
