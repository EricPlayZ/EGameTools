#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <format>
#include <cmath>
#include <Windows.h>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imguiex.h>
#include <EGSDK\Engine\CVideoSettings.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\Utils\Values.h>
#include <EGT\GamePH\Camera\CameraDollyRuntime.h>
#include <EGT\GamePH\Camera\CameraFOV.h>
#include <EGT\GamePH\Camera\CameraRuntime.h>
#include <EGT\Menu\Camera.h>

namespace EGT::Menu {
	namespace Camera {
		vec3 cameraOffset{};
		float firstPersonFOV = 0.0f;
		float originalFirstPersonFOVBeforeZoomIn = 0.0f;
		ImGui::KeyBindOption firstPersonZoomIn{ false, 'Q', ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "FirstPersonZoomInHoldingKey", ImGui::KeyBindBehavior::Action }, ImGui::ConfigBindingInfo{ "Camera:FirstPerson", "ZoomIn" }, false };
		Config::ConfigFloatRef cameraOffsetX{ "Camera:FirstPerson", "XOffset", cameraOffset.X };
		Config::ConfigFloatRef cameraOffsetY{ "Camera:FirstPerson", "YOffset", cameraOffset.Y };
		Config::ConfigFloatRef cameraOffsetZ{ "Camera:FirstPerson", "ZOffset", cameraOffset.Z };

		ImGui::Option photoMode{ false };

		ImGui::KeyBindOption freeCam{ false, VK_F3, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "FreeCamToggleKey" } };
		Config::ConfigFloat freeCamFOV{ "Camera:FreeCam", "FOV", 0.0f };
		Config::ConfigFloat freeCamSpeed{ "Camera:FreeCam", "Speed", 2.0f };
		ImGui::KeyBindOption teleportPlayerToCamera{ false, VK_F4, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "TeleportPlayerToCameraToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:FreeCam", "TeleportPlayerToCamera" } };

		ImGui::KeyBindOption thirdPersonCamera{ false, VK_F1, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "ThirdPersonToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:ThirdPerson", "Enabled" } };
		ImGui::KeyBindOption tpUseTPPModel{ false, VK_F2, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "UseTPPModelToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:ThirdPerson", "UseTPPModel" } };
		Config::ConfigFloat thirdPersonFOV{ "Camera:ThirdPerson", "FOV", 0.0f };
		Config::ConfigFloat thirdPersonDistanceBehindPlayer{ "Camera:ThirdPerson", "DistanceBehindPlayer", 2.0f };
		Config::ConfigFloat thirdPersonHeightAbovePlayer{ "Camera:ThirdPerson", "HeightAbovePlayer", 1.3f };
		Config::ConfigFloat thirdPersonHorizontalDistanceFromPlayer{ "Camera:ThirdPerson", "HorizontalDistanceFromPlayer", -0.6f };

		Config::ConfigFloat lensDistortion{ "Camera:Misc", "LensDistortion", 20.0f };
		float altLensDistortion = lensDistortion;
		ImGui::KeyBindOption goProMode{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "GoProMode" }, ImGui::ConfigBindingInfo{ "Camera:Misc", "GoProMode" } };
		ImGui::KeyBindOption disableSafezoneFOVReduction{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "DisableSafezoneFOVReduction" }, ImGui::ConfigBindingInfo{ "Camera:Misc", "DisableSafezoneFOVReduction" } };
		ImGui::KeyBindOption disablePhotoModeLimits{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "DisablePhotoModeLimits" }, ImGui::ConfigBindingInfo{ "Camera:Misc", "DisablePhotoModeLimits" } };
		ImGui::KeyBindOption disableHeadCorrection{ false, VK_NONE, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "DisableHeadCorrectionToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:Misc", "DisableHeadCorrection" } };

		ImGui::KeyBindOption dollyCamEnabled{ false, VK_F11, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "DollyCamToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "Enabled" } };
		ImGui::KeyBindOption dollyRecording{ false, VK_F12, ImGui::ConfigBindingInfo{ "Menu:Keybinds", "DollyCamRecordToggleKey" }, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "Recording" } };
		ImGui::KeyBindOption dollyPlayPause{ false, VK_HOME, ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "DollyCamPlayPauseKey", ImGui::KeyBindBehavior::Action }, false };
		ImGui::KeyBindOption dollyAddKeyframe{ false, VK_INSERT, ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "DollyCamAddKeyframeKey", ImGui::KeyBindBehavior::Action }, false };
		ImGui::KeyBindOption dollyPrevKeyframe{ false, VK_PRIOR, ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "DollyCamPrevKeyframeKey", ImGui::KeyBindBehavior::Action }, false };
		ImGui::KeyBindOption dollyNextKeyframe{ false, VK_NEXT, ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "DollyCamNextKeyframeKey", ImGui::KeyBindBehavior::Action }, false };
		ImGui::KeyBindOption dollyClearPath{ false, VK_DELETE, ImGui::ConfigBindingInfo{ "Camera:Hotkeys", "DollyCamClearPathKey", ImGui::KeyBindBehavior::Action }, false };

		ImGui::Option dollyLoop{ false, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "Loop" } };
		ImGui::Option dollyPlaying{ false };

		Config::ConfigFloat dollyPlaybackSpeed{ "Camera:DollyCam", "PlaybackSpeed", 1.0f };
		Config::ConfigFloat dollyRecordMinInterval{ "Camera:DollyCam", "RecordMinInterval", 0.20f };
		Config::ConfigFloat dollyRecordPositionThreshold{ "Camera:DollyCam", "RecordPositionThreshold", 0.30f };
		Config::ConfigFloat dollyRecordRotationThreshold{ "Camera:DollyCam", "RecordRotationThreshold", 6.0f };
		Config::ConfigFloat dollyRecordFOVThreshold{ "Camera:DollyCam", "RecordFOVThreshold", 2.0f };
		Config::ConfigFloat dollyDefaultKeyframeInterval{ "Camera:DollyCam", "DefaultKeyframeInterval", 0.5f };
		ImGui::Option dollyPauseRecordingTimeWhenIdle{ true, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "PauseRecordingTimeWhenIdle" } };
		ImGui::Option dollyDrawPath{ true, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "DrawPathOverlay" } };
		ImGui::Option dollyDrawInterpolatedPath{ true, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "DrawInterpolatedPath" } };
		Config::ConfigInt dollyTimingModel{ "Camera:DollyCam", "TimingModel", 0 };
		Config::ConfigFloat dollyTurnSmoothingAggressiveness{ "Camera:DollyCam", "TurnSmoothingAggressiveness", 0.35f };
		ImGui::Option dollyTurnSmoothingDeviateKeyframes{ true, ImGui::ConfigBindingInfo{ "Camera:DollyCam", "TurnSmoothingDeviateKeyframes" } };
		Config::ConfigFloat dollyCornerTangentScale{ "Camera:DollyCam", "CornerTangentScale", 1.0f };
		Config::ConfigFloat dollyOverlayVisualDensity{ "Camera:DollyCam", "OverlayVisualDensity", 0.65f };
		Config::ConfigInt dollyOverlaySampleBudget{ "Camera:DollyCam", "OverlaySampleBudget", 1024 };
		Config::ConfigInt dollyInterpolationCurve{ "Camera:DollyCam", "InterpolationCurve", 1 };
		float dollyTimelineTime = 0.0f;
		float dollyDuration = 0.0f;
		int dollySelectedKeyframe = -1;
		bool dollyClearPathConfirm = false;
		std::vector<DollyKeyframe> dollyKeyframes{};

		constexpr float kDollyMinTimeGap = 0.001f;
		constexpr float kDegToRad = 0.01745329251994329577f;

		void NormalizeDollyTimesFrom(const int startIndex) {
			if (dollyKeyframes.empty())
				return;
			const int first = std::clamp(startIndex, 0, static_cast<int>(dollyKeyframes.size()) - 1);
			dollyKeyframes[first].time = std::max(0.0f, dollyKeyframes[first].time);
			for (size_t i = static_cast<size_t>(first + 1); i < dollyKeyframes.size(); i++)
				dollyKeyframes[i].time = std::max(dollyKeyframes[i].time, dollyKeyframes[i - 1].time);
		}

		void RefreshDollyTrackState() {
			if (dollyKeyframes.empty()) {
				dollySelectedKeyframe = -1;
				dollyDuration = 0.0f;
				dollyTimelineTime = 0.0f;
				return;
			}

			NormalizeDollyTimesFrom(0);
			dollyDuration = std::max(0.0f, dollyKeyframes.back().time);
			dollyTimelineTime = std::clamp(dollyTimelineTime, 0.0f, dollyDuration);
			dollySelectedKeyframe = std::clamp(dollySelectedKeyframe, 0, static_cast<int>(dollyKeyframes.size()) - 1);
		}

		void SelectNearestKeyframeByTime() {
			if (dollyKeyframes.empty()) {
				dollySelectedKeyframe = -1;
				return;
			}

			float nearestDistance = std::abs(dollyKeyframes.front().time - dollyTimelineTime);
			int nearestIndex = 0;
			for (size_t i = 1; i < dollyKeyframes.size(); i++) {
				const float dist = std::abs(dollyKeyframes[i].time - dollyTimelineTime);
				if (dist < nearestDistance) {
					nearestDistance = dist;
					nearestIndex = static_cast<int>(i);
				}
			}
			dollySelectedKeyframe = nearestIndex;
		}

		void InsertKeyframe(const DollyKeyframe& keyframe, const int desiredIndex = -1) {
			if (desiredIndex >= 0 && desiredIndex <= static_cast<int>(dollyKeyframes.size()))
				dollyKeyframes.insert(dollyKeyframes.begin() + desiredIndex, keyframe);
			else
				dollyKeyframes.push_back(keyframe);
			NormalizeDollyTimesFrom(std::max(0, desiredIndex));
			RefreshDollyTrackState();
		}

		bool TryCaptureKeyframe(DollyKeyframe& outKeyframe) {
			if (!GamePH::Camera::CaptureCurrentDollyKeyframe(outKeyframe))
				return false;
			outKeyframe.time = dollyTimelineTime;
			return true;
		}

		void AddCurrentCameraAsKeyframe() {
			DollyKeyframe keyframe{};
			if (!TryCaptureKeyframe(keyframe))
				return;
			const float interval = std::max(0.01f, static_cast<float>(dollyDefaultKeyframeInterval));
			keyframe.time = dollyKeyframes.empty() ? 0.0f : dollyKeyframes.back().time + interval;

			int insertIndex = static_cast<int>(dollyKeyframes.size());
			for (size_t i = 0; i < dollyKeyframes.size(); i++) {
				if (dollyKeyframes[i].time > keyframe.time) {
					insertIndex = static_cast<int>(i);
					break;
				}
			}

			InsertKeyframe(keyframe, insertIndex);
			dollySelectedKeyframe = std::clamp(insertIndex, 0, static_cast<int>(dollyKeyframes.size()) - 1);
			dollyTimelineTime = keyframe.time;
		}

		void ApplySelectedKeyframeIfPossible() {
			if (dollySelectedKeyframe < 0 || dollySelectedKeyframe >= static_cast<int>(dollyKeyframes.size()))
				return;
			if (dollyPlaying.GetValue())
				return;

			dollyTimelineTime = dollyKeyframes[static_cast<size_t>(dollySelectedKeyframe)].time;
			DollyKeyframe evaluated{};
			if (GamePH::Camera::EvaluateDollyKeyframeAtTime(dollyTimelineTime, evaluated))
				GamePH::Camera::ApplyDollyKeyframe(evaluated);
			else
				GamePH::Camera::ApplyDollyKeyframe(dollyKeyframes[static_cast<size_t>(dollySelectedKeyframe)]);
		}

		void NormalizeDollyTimesByDistance() {
			if (dollyKeyframes.size() < 2)
				return;

			const float startTime = dollyKeyframes.front().time;
			const float endTime = dollyKeyframes.back().time;
			const float duration = std::max(0.01f, endTime - startTime);
			std::vector<float> cumulativeDistances(dollyKeyframes.size(), 0.0f);
			float totalDistance = 0.0f;
			for (size_t i = 1; i < dollyKeyframes.size(); i++) {
				const vec3 delta = dollyKeyframes[i].position - dollyKeyframes[i - 1].position;
				totalDistance += std::sqrt(delta.dot(delta));
				cumulativeDistances[i] = totalDistance;
			}

			if (totalDistance <= 0.0001f) {
				const float stepTime = duration / static_cast<float>(dollyKeyframes.size() - 1);
				for (size_t i = 1; i < dollyKeyframes.size(); i++)
					dollyKeyframes[i].time = startTime + stepTime * static_cast<float>(i);
			} else {
				for (size_t i = 1; i < dollyKeyframes.size(); i++) {
					const float normalizedDistance = cumulativeDistances[i] / totalDistance;
					dollyKeyframes[i].time = startTime + duration * normalizedDistance;
				}
			}
			RefreshDollyTrackState();
		}

		struct DollyProjectionContext {
			vec3 camPos{};
			vec3 camForward{};
			mtx34 viewMatrix{};
			float tanHalfFov = 0.0f;
			float invAspect = 1.0f;
			ImVec2 displaySize{};
			float margin = 1000.0f;
		};

		bool BuildDollyProjectionContext(DollyProjectionContext& outContext) {
			auto* iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return false;
			auto* viewCam = iLevel->GetViewCamera();
			if (!viewCam)
				return false;
			if (!viewCam->GetPosition(&outContext.camPos) || !viewCam->GetForwardVector(&outContext.camForward))
				return false;
			auto* viewMtx = viewCam->GetViewMatrix();
			if (!viewMtx)
				return false;

			outContext.viewMatrix = *viewMtx;
			const float fovDeg = std::max(10.0f, viewCam->GetFOV());
			outContext.tanHalfFov = std::tan((fovDeg * kDegToRad) * 0.5f);
			if (std::abs(outContext.tanHalfFov) < 0.00001f)
				return false;

			outContext.displaySize = ImGui::GetIO().DisplaySize;
			if (outContext.displaySize.x <= 1.0f || outContext.displaySize.y <= 1.0f)
				return false;
			outContext.invAspect = outContext.displaySize.y / outContext.displaySize.x;
			return true;
		}

		bool ProjectWorldToScreen(const DollyProjectionContext& context, const vec3& worldPos, ImVec2& outScreenPos) {
			const vec3 toPoint = worldPos - context.camPos;
			if (toPoint.dot(context.camForward) >= 0.0f)
				return false;

			const float camX = context.viewMatrix.Row1.X * worldPos.X + context.viewMatrix.Row1.Y * worldPos.Y + context.viewMatrix.Row1.Z * worldPos.Z + context.viewMatrix.Row1.W;
			const float camY = context.viewMatrix.Row2.X * worldPos.X + context.viewMatrix.Row2.Y * worldPos.Y + context.viewMatrix.Row2.Z * worldPos.Z + context.viewMatrix.Row2.W;
			const float camZRaw = context.viewMatrix.Row3.X * worldPos.X + context.viewMatrix.Row3.Y * worldPos.Y + context.viewMatrix.Row3.Z * worldPos.Z + context.viewMatrix.Row3.W;
			const float camZ = std::abs(camZRaw);
			if (!std::isfinite(camX) || !std::isfinite(camY) || !std::isfinite(camZ) || camZ <= 0.01f)
				return false;

			const float ndcX = camX / (camZ * context.tanHalfFov / context.invAspect);
			const float ndcY = camY / (camZ * context.tanHalfFov);
			if (!std::isfinite(ndcX) || !std::isfinite(ndcY))
				return false;
			if (std::abs(ndcX) > 4.0f || std::abs(ndcY) > 4.0f)
				return false;

			outScreenPos.x = (ndcX * 0.5f + 0.5f) * context.displaySize.x;
			outScreenPos.y = (-ndcY * 0.5f + 0.5f) * context.displaySize.y;
			if (!std::isfinite(outScreenPos.x) || !std::isfinite(outScreenPos.y))
				return false;
			if (outScreenPos.x < -context.margin || outScreenPos.x > context.displaySize.x + context.margin ||
				outScreenPos.y < -context.margin || outScreenPos.y > context.displaySize.y + context.margin)
				return false;
			return true;
		}

		vec3 ForwardFromEulerDeg(const vec3& eulerDeg) {
			const float yaw = eulerDeg.Y * kDegToRad;
			const float pitch = eulerDeg.X * kDegToRad;
			const float cosPitch = std::cos(pitch);
			const vec3 forward{
				std::cos(yaw) * cosPitch,
				std::sin(pitch),
				std::sin(yaw) * cosPitch
			};
			const float lenSq = forward.dot(forward);
			if (lenSq <= 0.00001f)
				return vec3(1.0f, 0.0f, 0.0f);
			return forward / std::sqrt(lenSq);
		}

		void DrawOrientationIndicator(ImDrawList* drawList, const DollyProjectionContext& projectionContext, const vec3& position, const vec3& rotation, const float length, const ImU32 color, const float thickness) {
			const vec3 viewForward = ForwardFromEulerDeg(rotation) * -1.0f;
			const vec3 endPos = position + viewForward * length;
			ImVec2 startScreen{};
			ImVec2 endScreen{};
			if (!ProjectWorldToScreen(projectionContext, position, startScreen) || !ProjectWorldToScreen(projectionContext, endPos, endScreen))
				return;
			drawList->AddLine(startScreen, endScreen, color, thickness);
			drawList->AddCircleFilled(endScreen, std::max(1.5f, thickness + 0.5f), color);
		}

		void DrawDollyPathOverlay() {
			if (!dollyDrawPath.GetValue() || dollyKeyframes.size() < 2)
				return;
			auto* iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded() || iLevel->IsTimerFrozen())
				return;

			ImDrawList* drawList = ImGui::GetBackgroundDrawList();
			if (!drawList)
				return;

			DollyProjectionContext projectionContext{};
			if (!BuildDollyProjectionContext(projectionContext))
				return;

			std::vector<ImVec2> polylinePoints{};
			polylinePoints.reserve(static_cast<size_t>(std::max(32, static_cast<int>(dollyKeyframes.size()) * 4)));
			const auto flushPolyline = [&]() {
				if (polylinePoints.size() >= 2)
					drawList->AddPolyline(polylinePoints.data(), static_cast<int>(polylinePoints.size()), IM_COL32(70, 200, 255, 190), ImDrawFlags_None, 2.0f);
				polylinePoints.clear();
			};

			if (dollyDrawInterpolatedPath.GetValue() && static_cast<int>(dollyInterpolationCurve) == 1 && dollyDuration > 0.0f) {
				const int sampleBudget = std::clamp(static_cast<int>(dollyOverlaySampleBudget), 64, 4096);
				const float densityMultiplier = std::clamp(static_cast<float>(dollyOverlayVisualDensity), 0.05f, 5.0f);
				const float spread = std::clamp(densityMultiplier, 0.05f, 1.0f);
				const int durationSamples = std::clamp(static_cast<int>(dollyDuration * 30.0f), 32, sampleBudget);
				const float sampleCountFloat = 24.0f + (static_cast<float>(durationSamples) - 24.0f) * spread;
				int sampleCount = std::clamp(static_cast<int>(sampleCountFloat), 16, sampleBudget);
				if (densityMultiplier > 1.0f)
					sampleCount = std::clamp(static_cast<int>(std::lround(static_cast<float>(sampleCount) * densityMultiplier)), 16, sampleBudget);
				float orientationMarkersFloat = 8.0f + (96.0f - 8.0f) * spread;
				if (densityMultiplier > 1.0f)
					orientationMarkersFloat *= densityMultiplier;
				const int maxOrientationMarkers = std::max(4, static_cast<int>(std::lround(orientationMarkersFloat)));
				const int orientationStride = std::max(1, sampleCount / maxOrientationMarkers);
				int upperIndexHint = 1;
				for (int i = 0; i <= sampleCount; i++) {
					const float t = (dollyDuration * static_cast<float>(i)) / static_cast<float>(sampleCount);
					DollyKeyframe evaluated{};
					if (!GamePH::Camera::EvaluateDollyKeyframeAtTimeWithHint(t, evaluated, upperIndexHint)) {
						flushPolyline();
						continue;
					}

					ImVec2 screenPos{};
					if (!ProjectWorldToScreen(projectionContext, evaluated.position, screenPos)) {
						flushPolyline();
						continue;
					}
					polylinePoints.push_back(screenPos);
					if (i % orientationStride == 0)
						DrawOrientationIndicator(drawList, projectionContext, evaluated.position, evaluated.rotation, 0.75f, IM_COL32(120, 255, 160, 165), 1.2f);
				}
				flushPolyline();
			} else {
				for (const auto& keyframe : dollyKeyframes) {
					ImVec2 screenPos{};
					if (!ProjectWorldToScreen(projectionContext, keyframe.position, screenPos)) {
						flushPolyline();
						continue;
					}
					polylinePoints.push_back(screenPos);
				}
				flushPolyline();
			}

			for (size_t i = 0; i < dollyKeyframes.size(); i++) {
				const auto& keyframe = dollyKeyframes[i];
				ImVec2 screenPos{};
				if (!ProjectWorldToScreen(projectionContext, keyframe.position, screenPos))
					continue;

				const bool isSelected = dollySelectedKeyframe == static_cast<int>(i);
				if (isSelected) {
					drawList->AddCircleFilled(screenPos, 5.0f, IM_COL32(255, 230, 70, 230));
					drawList->AddCircle(screenPos, 8.0f, IM_COL32(255, 230, 70, 200), 0, 2.0f);
					drawList->AddText(ImVec2(screenPos.x + 10.0f, screenPos.y - 22.0f), IM_COL32(255, 230, 70, 240), "SELECTED");
					DrawOrientationIndicator(drawList, projectionContext, keyframe.position, keyframe.rotation, 0.90f, IM_COL32(255, 230, 70, 235), 2.3f);
				} else {
					drawList->AddCircleFilled(screenPos, 4.0f, IM_COL32(230, 80, 80, 230));
					DrawOrientationIndicator(drawList, projectionContext, keyframe.position, keyframe.rotation, 0.80f, IM_COL32(255, 120, 220, 220), 1.9f);
				}

				char keyframeLabel[16]{};
				std::snprintf(keyframeLabel, sizeof(keyframeLabel), "#%zu", i);
				drawList->AddText(ImVec2(screenPos.x + 7.0f, screenPos.y + 6.0f), IM_COL32(255, 120, 120, 240), keyframeLabel);
			}
		}

		void RenderDollyManagerUI() {
			static const char* const interpolationCurveItems[] = {
				"Linear",
				"Smooth Path (Monotone Hermite)"
			};
			static const char* const timingModelItems[] = {
				"Time-Based",
				"Arc-Length (Constant Speed)"
			};
			constexpr ImGuiTreeNodeFlags headerOpen = ImGuiTreeNodeFlags_DefaultOpen;

			ImGui::SeparatorTextSection("Dolly Cam");
			if (!freeCam.GetValue())
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(220, 170, 60, 255)), "Enable Free Camera first to use DollyCam.");
			ImGui::BeginDisabled(dollyCamEnabled.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Enabled##DollyCam", &dollyCamEnabled, "Enable keyframed dolly camera playback/recording (requires Free Camera enabled)");
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!dollyCamEnabled.GetValue());

			if (ImGui::CollapsingHeaderSmooth("Playback & timeline##DollyCamPlayback", headerOpen)) {
				ImGui::CheckboxHotkey("Record##DollyCam", &dollyRecording, "Records keyframes over time at the current timeline cursor");
				if (ImGui::ButtonHotkey(dollyPlaying.GetValue() ? "Pause##DollyCam" : "Play##DollyCam", &dollyPlayPause, "Toggle DollyCam playback"))
					dollyPlaying.Toggle();
				ImGui::SameLine();
				if (ImGui::Button("Stop##DollyCam")) {
					dollyPlaying.SetValue(false);
					dollyTimelineTime = 0.0f;
				}
				ImGui::SameLine();
				ImGui::Checkbox("Loop##DollyCam", &dollyLoop);

				ImGui::SliderFloatStacked("Playback Speed##DollyCam", &dollyPlaybackSpeed, 0.1f, 4.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);

				ImGui::Text("Status: %s | Keyframes: %zu | Duration: %.2fs | Cursor: %.2fs",
					dollyRecording.GetValue() ? "Recording" : (dollyPlaying.GetValue() ? "Playing" : "Paused"),
					dollyKeyframes.size(),
					dollyDuration,
					dollyTimelineTime
				);

				bool changedTimeline = false;
				changedTimeline = ImGui::SliderFloat("Timeline##DollyCam", &dollyTimelineTime, 0.0f, std::max(0.1f, dollyDuration), "%.2fs", ImGuiSliderFlags_AlwaysClamp);
				if (changedTimeline) {
					SelectNearestKeyframeByTime();
					if (!dollyPlaying.GetValue()) {
						DollyKeyframe evaluated{};
						if (GamePH::Camera::EvaluateDollyKeyframeAtTime(dollyTimelineTime, evaluated))
							GamePH::Camera::ApplyDollyKeyframe(evaluated);
					}
				}

				const ImGuiStyle& style = ImGui::GetStyle();
				const float rowHeight = ImGui::GetTextLineHeightWithSpacing();
				const float listHeight = rowHeight * 6.0f + style.FramePadding.y * 2.0f;
				if (ImGui::BeginListBox("##DollyKeyframes", ImVec2(0.0f, listHeight))) {
					ImGuiListClipper clipper;
					const int count = static_cast<int>(dollyKeyframes.size());
					clipper.Begin(count, rowHeight);
					while (clipper.Step()) {
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
							ImGui::PushID(i);
							const auto& kf = dollyKeyframes[static_cast<size_t>(i)];
							const std::string rowLabel = std::format("#{:02d}  t:{:.2f}s  P({:.1f},{:.1f},{:.1f})  R({:.1f},{:.1f},{:.1f})  FOV:{:.1f}",
								i, kf.time, kf.position.X, kf.position.Y, kf.position.Z, kf.rotation.X, kf.rotation.Y, kf.rotation.Z, kf.fov);
							if (ImGui::SelectableSmooth(rowLabel.c_str(), dollySelectedKeyframe == i)) {
								dollySelectedKeyframe = i;
								ApplySelectedKeyframeIfPossible();
							}
							ImGui::PopID();
						}
					}
					ImGui::EndListBox();
				}

				if (ImGui::ButtonHotkey("Add Keyframe", &dollyAddKeyframe, "Capture current camera pose/FOV at the end of track using default interval spacing"))
					AddCurrentCameraAsKeyframe();

				ImGui::BeginDisabled(dollySelectedKeyframe < 0 || dollySelectedKeyframe >= static_cast<int>(dollyKeyframes.size()));
				if (ImGui::Button("Insert Before##DollyCam")) {
					DollyKeyframe keyframe{};
					if (TryCaptureKeyframe(keyframe)) {
						const int insertIndex = std::max(0, dollySelectedKeyframe);
						if (insertIndex > 0)
							keyframe.time = std::max(0.0f, dollyKeyframes[insertIndex - 1].time + kDollyMinTimeGap);
						keyframe.time = std::min(keyframe.time, dollyKeyframes[insertIndex].time);
						InsertKeyframe(keyframe, insertIndex);
						dollySelectedKeyframe = insertIndex;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Insert After##DollyCam")) {
					DollyKeyframe keyframe{};
					if (TryCaptureKeyframe(keyframe)) {
						const int insertIndex = std::min(static_cast<int>(dollyKeyframes.size()), dollySelectedKeyframe + 1);
						keyframe.time = dollyKeyframes[dollySelectedKeyframe].time + kDollyMinTimeGap;
						InsertKeyframe(keyframe, insertIndex);
						dollySelectedKeyframe = insertIndex;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Duplicate##DollyCam")) {
					const DollyKeyframe duplicated = dollyKeyframes[dollySelectedKeyframe];
					InsertKeyframe(duplicated, dollySelectedKeyframe + 1);
					dollySelectedKeyframe++;
				}

				if (ImGui::Button("Move Up##DollyCam") && dollySelectedKeyframe > 0) {
					std::iter_swap(dollyKeyframes.begin() + dollySelectedKeyframe, dollyKeyframes.begin() + dollySelectedKeyframe - 1);
					dollySelectedKeyframe--;
					RefreshDollyTrackState();
				}
				ImGui::SameLine();
				if (ImGui::Button("Move Down##DollyCam") && dollySelectedKeyframe < static_cast<int>(dollyKeyframes.size()) - 1) {
					std::iter_swap(dollyKeyframes.begin() + dollySelectedKeyframe, dollyKeyframes.begin() + dollySelectedKeyframe + 1);
					dollySelectedKeyframe++;
					RefreshDollyTrackState();
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete##DollyCam")) {
					dollyKeyframes.erase(dollyKeyframes.begin() + dollySelectedKeyframe);
					RefreshDollyTrackState();
				}

				if (ImGui::ButtonHotkey("Prev Keyframe", &dollyPrevKeyframe, "Jump to previous keyframe") && dollySelectedKeyframe > 0) {
					dollySelectedKeyframe--;
					ApplySelectedKeyframeIfPossible();
				}
				ImGui::SameLine();
				if (ImGui::ButtonHotkey("Next Keyframe", &dollyNextKeyframe, "Jump to next keyframe") && dollySelectedKeyframe >= 0 && dollySelectedKeyframe < static_cast<int>(dollyKeyframes.size()) - 1) {
					dollySelectedKeyframe++;
					ApplySelectedKeyframeIfPossible();
				}
				ImGui::EndDisabled();

				if (ImGui::ButtonHotkey("Clear Path##DollyCam", &dollyClearPath, "Removes all DollyCam keyframes"))
					ImGui::OpenPopup("Clear Dolly Path Confirmation");
				ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
				if (ImGui::BeginPopupModal("Clear Dolly Path Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::Text("Delete all DollyCam keyframes?");
					if (ImGui::Button("Clear All##DollyCamConfirm", ImVec2(220.0f, 0.0f) * Menu::scale)) {
						dollyKeyframes.clear();
						RefreshDollyTrackState();
						dollyPlaying.SetValue(false);
						dollyRecording.SetValue(false);
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel##DollyCamConfirm", ImVec2(220.0f, 0.0f) * Menu::scale))
						ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
			}

			if (ImGui::CollapsingHeaderSmooth("Recording##DollyCamRec", ImGuiTreeNodeFlags_None)) {
				ImGui::SliderFloatStacked("Record Min Interval##DollyCam", &dollyRecordMinInterval, 0.05f, 1.0f, "%.2fs", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloatStacked("Record Pos Threshold##DollyCam", &dollyRecordPositionThreshold, 0.001f, 1000.0f, "%.3fm", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloatStacked("Record Rot Threshold##DollyCam", &dollyRecordRotationThreshold, 0.001f, 180.0f, "%.3f deg", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloatStacked("Record FOV Threshold##DollyCam", &dollyRecordFOVThreshold, 0.001f, 180.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				dollyRecordPositionThreshold = std::max(0.001f, static_cast<float>(dollyRecordPositionThreshold));
				dollyRecordRotationThreshold = std::max(0.001f, static_cast<float>(dollyRecordRotationThreshold));
				dollyRecordFOVThreshold = std::max(0.001f, static_cast<float>(dollyRecordFOVThreshold));
				ImGui::SliderFloatStacked("Default Keyframe Interval##DollyCam", &dollyDefaultKeyframeInterval, 0.05f, 10.0f, "%.2fs", ImGuiSliderFlags_AlwaysClamp);
				ImGui::Checkbox("Pause recording time while idle##DollyCam", &dollyPauseRecordingTimeWhenIdle);
			}

			if (ImGui::CollapsingHeaderSmooth("Path & smoothing##DollyCamPath", headerOpen)) {
				ImGui::ComboStacked("Timing Model##DollyCam", &dollyTimingModel, timingModelItems, IM_ARRAYSIZE(timingModelItems));
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip("Time-Based: cursor time maps directly to keyframe times.\nArc-Length: cursor moves you at constant speed along the sampled path (independent of uneven keyframe spacing).");

				if (ImGui::Button("Normalize Time By Distance##DollyCam")) {
					NormalizeDollyTimesByDistance();
					if (!dollyPlaying.GetValue())
						ApplySelectedKeyframeIfPossible();
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip(
						"Rewrites keyframe timestamps so each segment's duration matches its share of total path length.\n"
						"Most useful for Time-Based playback (smoother real speed when intervals were uneven).\n"
						"Arc-Length playback already follows distance at constant speed; you can still use this to clean up the timeline or before switching timing modes."
					);

				ImGui::ComboStacked("Interpolation Curve##DollyCam", &dollyInterpolationCurve, interpolationCurveItems, IM_ARRAYSIZE(interpolationCurveItems));
				ImGui::SliderFloatStacked("Turn Smoothing##DollyCam", &dollyTurnSmoothingAggressiveness, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::Checkbox("Turn smoothing can deviate from keyframes##DollyCam", &dollyTurnSmoothingDeviateKeyframes);
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip("When enabled, smoothing uses softened guide points and can shift the path away from raw keyframe positions.\nWhen disabled, the path still passes through each keyframe; use Corner Tangent Scale to bow corners without moving keyframes.");
				ImGui::SliderFloatStacked("Corner Tangent Scale##DollyCam", &dollyCornerTangentScale, 0.0f, 2.5f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip("Scales Hermite position tangents at keyframes (smooth path only). >1 rounds corners more while still hitting keyframe positions when deviation is off.");
			}

			if (ImGui::CollapsingHeaderSmooth("Overlay##DollyCamOvl", ImGuiTreeNodeFlags_None)) {
				ImGui::Checkbox("Draw 3D Path Overlay##DollyCam", &dollyDrawPath);
				ImGui::Checkbox("Draw Interpolated Path##DollyCam", &dollyDrawInterpolatedPath);
				ImGui::SliderFloatStacked("Overlay Path / Orientation Density##DollyCam", &dollyOverlayVisualDensity, 0.05f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
					ImGui::SetTooltip("Up to 1: scales from sparse to the usual full-density baseline. Above 1: up to 5× that baseline for both path and orientation ticks (still capped by Overlay Sample Budget).");
				ImGui::SliderIntStacked("Overlay Sample Budget##DollyCam", &dollyOverlaySampleBudget, 64, 4096, "%d", ImGuiSliderFlags_AlwaysClamp);
			}

			if (ImGui::CollapsingHeaderSmooth("Selected keyframe##DollyCamSel", headerOpen)) {
				ImGui::BeginDisabled(dollySelectedKeyframe < 0 || dollySelectedKeyframe >= static_cast<int>(dollyKeyframes.size()));
				if (dollySelectedKeyframe >= 0 && dollySelectedKeyframe < static_cast<int>(dollyKeyframes.size())) {
					auto& selected = dollyKeyframes[dollySelectedKeyframe];
					const float prevSelectedTime = selected.time;
					if (ImGui::DragFloatStacked("Time##DollyCam", &selected.time, 0.02f, 0.0f, FLT_MAX, "%.2fs", ImGuiSliderFlags_AlwaysClamp)) {
						if (dollySelectedKeyframe > 0)
							selected.time = std::max(selected.time, dollyKeyframes[dollySelectedKeyframe - 1].time);
						if (selected.time > prevSelectedTime)
							NormalizeDollyTimesFrom(dollySelectedKeyframe);
						RefreshDollyTrackState();
					}
					ImGui::InputFloat3Stacked("Position (XYZ)##DollyCam", reinterpret_cast<float*>(&selected.position), "%.2fm");
					ImGui::InputFloat3Stacked("Rotation (Pitch,Yaw,Roll)##DollyCam", reinterpret_cast<float*>(&selected.rotation), "%.2f");
					ImGui::SliderFloat("FOV##DollyCam", &selected.fov, 20.0f, 160.0f, "%.1f");
					if (ImGui::Button("Snap Selected To Current Camera##DollyCam")) {
						DollyKeyframe captured{};
						if (GamePH::Camera::CaptureCurrentDollyKeyframe(captured)) {
							selected.position = captured.position;
							selected.rotation = captured.rotation;
							selected.fov = captured.fov;
						}
					}
					if (!dollyPlaying.GetValue() && ImGui::Button("Apply Selected Keyframe##DollyCam")) {
						DollyKeyframe evaluated{};
						if (GamePH::Camera::EvaluateDollyKeyframeAtTime(selected.time, evaluated))
							GamePH::Camera::ApplyDollyKeyframe(evaluated);
						else
							GamePH::Camera::ApplyDollyKeyframe(selected);
					}
				} else {
					ImGui::TextDisabled("Select a keyframe in the list above.");
				}
				ImGui::EndDisabled();
			}

			ImGui::EndDisabled();
		}

		void RenderDollyPathOverlay() {
			DrawDollyPathOverlay();
		}

		Tab Tab::instance{};
		void Tab::Init() {}
		void Tab::Update() {
			GamePH::Camera::UpdateFOVState();
			GamePH::Camera::UpdateRuntimeState();
		}

		void Tab::Render() {
			ImGui::SeparatorTextSection("First Person Camera", false);
			auto* pCVideoSettings = EGSDK::Engine::CVideoSettings::Get();
			const float baseFOV = GamePH::Camera::GetBaseFOV();
			const bool isZoomingIn = GamePH::Camera::IsZoomingIn();
			ImGui::BeginDisabled(!pCVideoSettings || EGSDK::Utils::Values::are_samef(baseFOV, 0.0f) || goProMode.GetValue() || isZoomingIn || dollyPlaying.GetValue());
			if (ImGui::SliderFloat("FOV##FirstPerson", "First person camera field of view", &firstPersonFOV, 20.0f, 160.0f, "%.0f") && pCVideoSettings)
				pCVideoSettings->extraFOV = firstPersonFOV - baseFOV;
			else if (pCVideoSettings && !goProMode.GetValue() && !EGSDK::Utils::Values::are_samef(baseFOV, 0.0f))
				firstPersonFOV = pCVideoSettings->extraFOV + baseFOV;
			ImGui::EndDisabled();

			ImGui::BeginDisabled(freeCam.GetChangesAreDisabled());
			ImGui::SliderFloat3Stacked("Camera Offset (XYZ)", reinterpret_cast<float*>(&cameraOffset), -0.5f, 0.5f, "%.2fm");
			ImGui::EndDisabled();
			ImGui::CheckboxHotkey("Zoom In", &firstPersonZoomIn, "Allows zooming in with the specified hotkey and changing zoom level with the mouse wheel");

			ImGui::SeparatorTextSection("Third Person Camera");
			ImGui::BeginDisabled(thirdPersonCamera.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Enabled##ThirdPerson", &thirdPersonCamera, "Enables the third person camera");
			ImGui::EndDisabled();
			ImGui::BeginDisabled(tpUseTPPModel.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Use Third Person Player (TPP) Model", &tpUseTPPModel, "Uses Aiden's TPP (Third Person Player) model while the third person camera is enabled");
			ImGui::EndDisabled();

			ImGui::BeginDisabled(EGSDK::Utils::Values::are_samef(baseFOV, 0.0f));
			ImGui::SliderFloat("FOV##ThirdPerson", "Third person camera field of view", &thirdPersonFOV, 20.0f, 160.0f, "%.0f");
			ImGui::EndDisabled();
			ImGui::SliderFloatStacked("Distance behind player", &thirdPersonDistanceBehindPlayer, 1.0f, 10.0f, "%.2fm");
			ImGui::SliderFloatStacked("Height above player", &thirdPersonHeightAbovePlayer, 1.0f, 3.0f, "%.2fm");
			ImGui::SliderFloatStacked("Horizontal distance from player", &thirdPersonHorizontalDistanceFromPlayer, -2.0f, 2.0f, "%.2fm");

			ImGui::SeparatorTextSection("Free Camera");
			ImGui::BeginDisabled(freeCam.GetChangesAreDisabled() || photoMode.GetValue());
			ImGui::CheckboxHotkey("Enabled##FreeCam", &freeCam, "Enables free camera which allows you to travel anywhere with the camera");
			ImGui::EndDisabled();
			ImGui::BeginDisabled(teleportPlayerToCamera.GetChangesAreDisabled());
			ImGui::CheckboxHotkey("Teleport Player to Camera", &teleportPlayerToCamera, "Teleports the player to the camera while Free Camera is activated");
			ImGui::EndDisabled();

			ImGui::BeginDisabled(EGSDK::Utils::Values::are_samef(baseFOV, 0.0f));
			if (ImGui::SliderFloat("FOV##FreeCam", "Free camera field of view", &freeCamFOV, 20.0f, 160.0f, "%.0f") &&
				dollySelectedKeyframe >= 0 && dollySelectedKeyframe < static_cast<int>(dollyKeyframes.size()))
				dollyKeyframes[static_cast<size_t>(dollySelectedKeyframe)].fov = freeCamFOV;
			ImGui::EndDisabled();
			ImGui::SliderFloatStacked("Speed##FreeCam", &freeCamSpeed, 0.1f, 200.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);

			RenderDollyManagerUI();

			ImGui::SeparatorTextSection("Misc");
			ImGui::BeginDisabled(goProMode.GetValue());
			ImGui::SliderFloat("Lens Distortion", "Default game value is 20%", goProMode.GetValue() ? &altLensDistortion : &lensDistortion, 0.0f, 100.0f, "%.1f%%");
			ImGui::EndDisabled();

			ImGui::CheckboxHotkey("GoPro Mode *", &goProMode, "Makes the camera behave similar to a GoPro mounted on the forehead");
			ImGui::CheckboxHotkey("Disable Safezone FOV Reduction", &disableSafezoneFOVReduction, "Disables the FOV reduction that happens while you're in a safezone");
			ImGui::CheckboxHotkey("Disable Photo Mode Limits", &disablePhotoModeLimits, "Disables the invisible box while in Photo Mode");
			ImGui::CheckboxHotkey("Disable Head Correction", &disableHeadCorrection, "Disables centering of the player's hands to the center of the camera");

			ImGui::Separator();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "* GoPro Mode is best used with Head Bob Reduction set to 0 and Player FOV\nCorrection set to 0 in game options");
		}
	}
}
