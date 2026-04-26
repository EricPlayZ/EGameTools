#include <EGT\GamePH\Camera\CameraRuntime_Internal.h>
#include <EGT\GamePH\Camera\CameraRuntime.h>

namespace EGT::GamePH::Camera {
	void UpdateRuntimeState() {
		UpdateFreeCamRuntime();
		UpdateTPPModelRuntime();
		UpdateCameraPlayerVarsRuntime();
		UpdateCameraDollyRuntime();
		UpdateCameraDisabledOptionsRuntime();
		UpdateCameraTogglesRuntime();
	}
}
