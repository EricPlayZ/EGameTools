#pragma once
#include <EGT\Menu\Camera.h>

namespace EGT::GamePH::Camera {
	bool CaptureCurrentDollyKeyframe(Menu::Camera::DollyKeyframe& outKeyframe);
	bool ApplyDollyKeyframe(const Menu::Camera::DollyKeyframe& keyframe);
	bool EvaluateDollyKeyframeAtTime(float timelineTime, Menu::Camera::DollyKeyframe& outKeyframe);
	bool EvaluateDollyKeyframeAtTimeWithHint(float timelineTime, Menu::Camera::DollyKeyframe& outKeyframe, int& inOutUpperIndex);
	void UpdateCameraDollyRuntime();
}
