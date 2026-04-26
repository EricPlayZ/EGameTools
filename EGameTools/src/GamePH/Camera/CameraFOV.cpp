#include <EGT\GamePH\Camera\CameraFOV.h>
#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>

namespace EGT::GamePH::Camera {
	void UpdateFOVState() {
		UpdateCameraBaseFOVRuntime();
		UpdateCameraGoProFOVRuntime();
		UpdateCameraZoomFOVRuntime();
		UpdateCameraFirstPersonFOVSyncRuntime();
	}

	float GetBaseFOV() {
		return baseFOVState;
	}

	bool IsZoomingIn() {
		return isZoomingInState;
	}
}
