#pragma once

namespace EGT::GamePH::Camera {
	extern float baseFOVState;
	extern bool isZoomingInState;

	extern void UpdateCameraBaseFOVRuntime();
	extern void UpdateCameraGoProFOVRuntime();
	extern void UpdateCameraZoomFOVRuntime();
	extern void UpdateCameraFirstPersonFOVSyncRuntime();

	extern void UpdateFreeCamRuntime();
	extern void UpdateTPPModelRuntime();
	extern void UpdateCameraPlayerVarsRuntime();
	extern void UpdateCameraDollyRuntime();
	extern void UpdateCameraDisabledOptionsRuntime();
	extern void UpdateCameraTogglesRuntime();
}
