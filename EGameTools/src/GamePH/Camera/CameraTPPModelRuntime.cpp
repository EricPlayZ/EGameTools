#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateTPPModelRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		auto* pPlayerDI_PH = EGSDK::GamePH::PlayerDI_PH::Get();
		if (!pPlayerDI_PH)
			return;

		if (Menu::Camera::freeCam.GetValue() && !iLevel->IsTimerFrozen())
			EGSDK::GamePH::ShowTPPModel(true);
		else if (Menu::Camera::freeCam.GetValue() && iLevel->IsTimerFrozen() && !Menu::Camera::photoMode.GetValue())
			EGSDK::GamePH::ShowTPPModel(false);
		else if (Menu::Camera::thirdPersonCamera.GetValue() && Menu::Camera::tpUseTPPModel.GetValue())
			EGSDK::GamePH::ShowTPPModel(true);
		else if (!Menu::Camera::photoMode.GetValue())
			EGSDK::GamePH::ShowTPPModel(false);
	}
}
