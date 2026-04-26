#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateCameraGoProFOVRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return;

		static float previousFirstPersonFOV = Menu::Camera::firstPersonFOV;
		if (Menu::Camera::goProMode.GetValue()) {
			if (Menu::Camera::goProMode.HasChangedTo(true)) {
				previousFirstPersonFOV = viewCam->GetFOV();
				Menu::Camera::goProMode.SetPrevValue(true);
			}

			viewCam->SetFOV(110.0f);
			Menu::Camera::firstPersonFOV = 110.0f;
		} else if (Menu::Camera::goProMode.HasChangedTo(false)) {
			Menu::Camera::firstPersonFOV = previousFirstPersonFOV;
			Menu::Camera::goProMode.SetPrevValue(false);
		}
	}
}
