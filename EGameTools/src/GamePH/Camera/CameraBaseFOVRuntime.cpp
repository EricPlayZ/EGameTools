#include <cmath>
#include <EGSDK\Engine\CVideoSettings.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	void UpdateCameraBaseFOVRuntime() {
		if (!EGSDK::Utils::Values::are_samef(baseFOVState, 0.0f))
			return;

		auto* pCVideoSettings = EGSDK::Engine::CVideoSettings::Get();
		if (!pCVideoSettings)
			return;
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;
		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return;

		baseFOVState = std::roundf(viewCam->GetFOV() - pCVideoSettings->extraFOV);
		Menu::Camera::firstPersonFOV = baseFOVState;
		if (EGSDK::Utils::Values::are_samef(Menu::Camera::thirdPersonFOV, 0.0f))
			Menu::Camera::thirdPersonFOV = baseFOVState;
		if (EGSDK::Utils::Values::are_samef(Menu::Camera::freeCamFOV, 0.0f))
			Menu::Camera::freeCamFOV = baseFOVState;
	}
}
