#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	static constexpr float baseSafezoneFOVReduction = -10.0f;
	static constexpr float baseSprintHeadCorrectionFactor = 0.55f;

	void UpdateCameraPlayerVarsRuntime() {
		if (!EGSDK::GamePH::PlayerVariables::gotPlayerVars)
			return;

		EGSDK::GamePH::PlayerVariables::ManageVarByBool("CameraDefaultFOVReduction", 0.0f, baseSafezoneFOVReduction, Menu::Camera::disableSafezoneFOVReduction.GetValue(), true);

		static float prevLensDistortion = Menu::Camera::lensDistortion;
		static bool lensDistortionJustEnabled = false;

		if (Menu::Camera::goProMode.GetValue()) {
			if (!lensDistortionJustEnabled) {
				prevLensDistortion = Menu::Camera::lensDistortion;
				lensDistortionJustEnabled = true;
			}
			Menu::Camera::altLensDistortion = 100.0f;
		} else if (lensDistortionJustEnabled) {
			Menu::Camera::altLensDistortion = prevLensDistortion;
			lensDistortionJustEnabled = false;
		}

		if (auto fovCorrectionVar = EGSDK::GamePH::PlayerVariables::GetVarRef("FOVCorrection"))
			fovCorrectionVar->SetValue(Menu::Camera::goProMode.GetValue() ? (Menu::Camera::altLensDistortion / 100.0f) : (Menu::Camera::lensDistortion / 100.0f));
		EGSDK::GamePH::PlayerVariables::ManageVarByBool("SprintHeadCorrectionFactor", 0.0f, baseSprintHeadCorrectionFactor, Menu::Camera::goProMode.GetValue() ? Menu::Camera::goProMode.GetValue() : Menu::Camera::disableHeadCorrection.GetValue(), true);
	}
}
