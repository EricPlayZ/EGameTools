#include <EGSDK\Engine\CVideoSettings.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Menu.h>

namespace EGT::GamePH::Camera {
	void UpdateCameraFirstPersonFOVSyncRuntime() {
		auto* videoSettings = EGSDK::Engine::CVideoSettings::Get();
		if (videoSettings && !EGSDK::Utils::Values::are_samef(baseFOVState, 0.0f) && !Menu::Camera::goProMode.GetValue() && !Menu::menuToggle.GetValue() && !isZoomingInState)
			Menu::Camera::firstPersonFOV = videoSettings->extraFOV + baseFOVState;
	}
}
