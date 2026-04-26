#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Camera\CameraFOV.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateCameraDisabledOptionsRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		Menu::Camera::freeCam.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded() || Menu::Camera::photoMode.GetValue() || IsZoomingIn());
		Menu::Camera::thirdPersonCamera.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded() || Menu::Camera::freeCam.GetValue() || Menu::Camera::photoMode.GetValue() || IsZoomingIn());
		Menu::Camera::tpUseTPPModel.SetChangesAreDisabled(Menu::Camera::freeCam.GetValue() || Menu::Camera::photoMode.GetValue());
		Menu::Camera::dollyCamEnabled.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded() || !Menu::Camera::freeCam.GetValue() || Menu::Camera::thirdPersonCamera.GetValue() || Menu::Camera::photoMode.GetValue() || IsZoomingIn());
		if (Menu::Camera::dollyCamEnabled.GetChangesAreDisabled()) {
			Menu::Camera::dollyRecording.SetValue(false);
			Menu::Camera::dollyPlaying.SetValue(false);
		}
	}

	void UpdateCameraTogglesRuntime() {
		if (Menu::Camera::goProMode.HasChanged()) {
			Menu::Camera::goProMode.SetPrevValue(Menu::Camera::goProMode.GetValue());
			EGSDK::GamePH::ReloadJumps();
		}
		if (Menu::Camera::disableHeadCorrection.HasChanged()) {
			Menu::Camera::disableHeadCorrection.SetPrevValue(Menu::Camera::disableHeadCorrection.GetValue());
			EGSDK::GamePH::ReloadJumps();
		}
	}
}
