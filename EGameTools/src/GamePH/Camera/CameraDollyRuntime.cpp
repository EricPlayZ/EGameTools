#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <ImGui\imgui.h>
#include <EGSDK\Engine\IBaseCamera.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\Utils\Values.h>
#include <EGT\GamePH\Camera\CameraDollyRuntime.h>
#include <EGT\GamePH\Camera\CameraFOV.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Menu.h>

namespace EGT::GamePH::Camera {
	constexpr float kMinTrackDuration = 0.001f;
	constexpr float kEpsilon = 0.0001f;
	constexpr float kDegToRad = 0.01745329251994329577f;
	constexpr float kRadToDeg = 57.295779513082320876f;
	constexpr int kTimingModelTimeBased = 0;
	constexpr int kTimingModelArcLength = 1;
	constexpr int kArcLengthSampleMin = 128;
	constexpr int kArcLengthSampleMax = 2048;

	struct ArcLengthSample {
		float distance = 0.0f;
		float rawTime = 0.0f;
	};

	struct ArcLengthCache {
		std::vector<ArcLengthSample> samples{};
		float totalDistance = 0.0f;
		uint64_t keyHash = 0;
		int interpolationCurve = 0;
		float smoothingAggressiveness = 0.0f;
		bool turnSmoothingDeviateKeyframes = false;
		float cornerTangentScale = 1.0f;
		bool valid = false;
	};
	ArcLengthCache arcLengthCache{};

	float Clamp01(const float value) {
		return std::clamp(value, 0.0f, 1.0f);
	}

	float LerpFloat(const float a, const float b, const float alpha) {
		return a + (b - a) * alpha;
	}

	float CatmullRomScalar(const float p0, const float p1, const float p2, const float p3, const float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return 0.5f * (
			(2.0f * p1) +
			(-p0 + p2) * t +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
		);
	}

	vec3 CatmullRomVec3(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, const float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return (p1 * 2.0f +
			(p2 - p0) * t +
			(p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
			(-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
	}

	float HermiteScalar(const float p1, const float p2, const float m1, const float m2, const float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return (2.0f * t3 - 3.0f * t2 + 1.0f) * p1 +
			(t3 - 2.0f * t2 + t) * m1 +
			(-2.0f * t3 + 3.0f * t2) * p2 +
			(t3 - t2) * m2;
	}

	vec3 HermiteVec3(const vec3& p1, const vec3& p2, const vec3& m1, const vec3& m2, const float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return p1 * (2.0f * t3 - 3.0f * t2 + 1.0f) +
			m1 * (t3 - 2.0f * t2 + t) +
			p2 * (-2.0f * t3 + 3.0f * t2) +
			m2 * (t3 - t2);
	}

	float SafeDelta(const float a, const float b) {
		return std::max(kMinTrackDuration, b - a);
	}

	float ComputeMonotoneSlope(const float pPrev, const float p, const float pNext, const float dtPrev, const float dtNext) {
		if (dtPrev <= kMinTrackDuration || dtNext <= kMinTrackDuration)
			return 0.0f;

		const float dPrev = (p - pPrev) / dtPrev;
		const float dNext = (pNext - p) / dtNext;
		if (dPrev * dNext <= 0.0f)
			return 0.0f;

		const float w1 = 2.0f * dtNext + dtPrev;
		const float w2 = dtNext + 2.0f * dtPrev;
		return (w1 + w2) / ((w1 / dPrev) + (w2 / dNext));
	}

	vec3 ComputeMonotoneSlopeVec3(const vec3& pPrev, const vec3& p, const vec3& pNext, const float dtPrev, const float dtNext) {
		return vec3(
			ComputeMonotoneSlope(pPrev.X, p.X, pNext.X, dtPrev, dtNext),
			ComputeMonotoneSlope(pPrev.Y, p.Y, pNext.Y, dtPrev, dtNext),
			ComputeMonotoneSlope(pPrev.Z, p.Z, pNext.Z, dtPrev, dtNext)
		);
	}

	float NormalizeDegrees(float value) {
		while (value > 180.0f)
			value -= 360.0f;
		while (value < -180.0f)
			value += 360.0f;
		return value;
	}

	float UnwrapAngleNear(const float value, const float reference) {
		return reference + NormalizeDegrees(value - reference);
	}

	vec3 LerpVec3(const vec3& a, const vec3& b, const float alpha) {
		return a + (b - a) * alpha;
	}

	vec3 GetSoftenedGuidePosition(const std::vector<Menu::Camera::DollyKeyframe>& keyframes, const int index, const float smoothingAggressiveness) {
		if (!Menu::Camera::dollyTurnSmoothingDeviateKeyframes.GetValue())
			return keyframes[static_cast<size_t>(index)].position;
		if (index <= 0 || index >= static_cast<int>(keyframes.size()) - 1 || smoothingAggressiveness <= 0.0f)
			return keyframes[static_cast<size_t>(index)].position;

		const vec3& current = keyframes[static_cast<size_t>(index)].position;
		const vec3& prev = keyframes[static_cast<size_t>(index - 1)].position;
		const vec3& next = keyframes[static_cast<size_t>(index + 1)].position;
		const vec3 localAverage = (prev + next) * 0.5f;
		return LerpVec3(current, localAverage, Clamp01(smoothingAggressiveness));
	}

	float LerpAngleDegrees(float a, float b, const float alpha) {
		const float delta = NormalizeDegrees(b - a);
		return NormalizeDegrees(a + delta * alpha);
	}

	vec3 NormalizeVecOrFallback(const vec3& vector, const vec3& fallback) {
		const float lengthSq = vector.dot(vector);
		if (lengthSq < kEpsilon)
			return fallback;
		return vector / std::sqrt(lengthSq);
	}

	vec3 RotateAroundAxis(const vec3& vector, const vec3& axisNormalized, const float angleRad) {
		const float c = std::cos(angleRad);
		const float s = std::sin(angleRad);
		const vec3 axis = NormalizeVecOrFallback(axisNormalized, vec3(0.0f, 1.0f, 0.0f));
		return vector * c + axis.cross(vector) * s + axis * (axis.dot(vector) * (1.0f - c));
	}

	vec3 ForwardFromEulerDeg(const vec3& eulerDeg) {
		const float yaw = eulerDeg.Y * kDegToRad;
		const float pitch = eulerDeg.X * kDegToRad;
		const float cosPitch = std::cos(pitch);
		return NormalizeVecOrFallback(vec3(
			std::cos(yaw) * cosPitch,
			std::sin(pitch),
			std::sin(yaw) * cosPitch
		), vec3(1.0f, 0.0f, 0.0f));
	}

	void BuildAxesFromEulerDeg(const vec3& eulerDeg, vec3& outLeft, vec3& outUp, vec3& outForward) {
		outForward = ForwardFromEulerDeg(eulerDeg);
		vec3 left = vec3(0.0f, 1.0f, 0.0f).cross(outForward);
		if (left.dot(left) < kEpsilon)
			left = vec3(1.0f, 0.0f, 0.0f);
		left = NormalizeVecOrFallback(left, vec3(1.0f, 0.0f, 0.0f));

		vec3 up = NormalizeVecOrFallback(outForward.cross(left), vec3(0.0f, 1.0f, 0.0f));
		const float roll = eulerDeg.Z * kDegToRad;
		if (std::abs(roll) > kEpsilon) {
			left = RotateAroundAxis(left, outForward, roll);
			up = RotateAroundAxis(up, outForward, roll);
		}

		outLeft = NormalizeVecOrFallback(left, vec3(1.0f, 0.0f, 0.0f));
		outUp = NormalizeVecOrFallback(up, vec3(0.0f, 1.0f, 0.0f));
	}

	mtx34 BuildInvCameraMatrix(const vec3& position, const vec3& eulerDeg) {
		vec3 left{}, up{}, forward{};
		BuildAxesFromEulerDeg(eulerDeg, left, up, forward);
		return mtx34(
			vec4(left.X, up.X, forward.X, position.X),
			vec4(left.Y, up.Y, forward.Y, position.Y),
			vec4(left.Z, up.Z, forward.Z, position.Z)
		);
	}

	mtx34 BuildViewCameraMatrix(const vec3& position, const vec3& eulerDeg) {
		vec3 left{}, up{}, forward{};
		BuildAxesFromEulerDeg(eulerDeg, left, up, forward);
		return mtx34(
			vec4(left.X, left.Y, left.Z, -left.dot(position)),
			vec4(up.X, up.Y, up.Z, -up.dot(position)),
			vec4(forward.X, forward.Y, forward.Z, -forward.dot(position))
		);
	}

	vec3 EulerFromVectors(const vec3& forwardVec, const vec3& upVec) {
		const vec3 forward = NormalizeVecOrFallback(forwardVec, vec3(1.0f, 0.0f, 0.0f));
		const vec3 up = NormalizeVecOrFallback(upVec, vec3(0.0f, 1.0f, 0.0f));

		vec3 result{};
		result.Y = std::atan2(forward.Z, forward.X) * kRadToDeg;
		result.X = std::asin(std::clamp(forward.Y, -1.0f, 1.0f)) * kRadToDeg;

		vec3 baseLeft = vec3(0.0f, 1.0f, 0.0f).cross(forward);
		if (baseLeft.dot(baseLeft) < kEpsilon)
			baseLeft = vec3(1.0f, 0.0f, 0.0f);
		baseLeft = NormalizeVecOrFallback(baseLeft, vec3(1.0f, 0.0f, 0.0f));
		const vec3 baseUp = NormalizeVecOrFallback(forward.cross(baseLeft), vec3(0.0f, 1.0f, 0.0f));
		const float sinRoll = forward.dot(baseUp.cross(up));
		const float cosRoll = std::clamp(baseUp.dot(up), -1.0f, 1.0f);
		result.Z = std::atan2(sinRoll, cosRoll) * kRadToDeg;

		result.X = NormalizeDegrees(result.X);
		result.Y = NormalizeDegrees(result.Y);
		result.Z = NormalizeDegrees(result.Z);
		return result;
	}

	bool IsDollyRuntimeUnavailable() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return true;
		if (!Menu::Camera::freeCam.GetValue())
			return true;
		if (Menu::Camera::dollyCamEnabled.GetChangesAreDisabled())
			return true;
		if (!Menu::Camera::dollyCamEnabled.GetValue())
			return true;
		return false;
	}

	void ClampTimelineState() {
		if (Menu::Camera::dollyKeyframes.empty()) {
			Menu::Camera::dollyTimelineTime = 0.0f;
			Menu::Camera::dollyDuration = 0.0f;
			Menu::Camera::dollySelectedKeyframe = -1;
			return;
		}

		Menu::Camera::dollyDuration = std::max(0.0f, Menu::Camera::dollyKeyframes.back().time);
		Menu::Camera::dollyTimelineTime = std::clamp(Menu::Camera::dollyTimelineTime, 0.0f, Menu::Camera::dollyDuration);
		Menu::Camera::dollySelectedKeyframe = std::clamp(Menu::Camera::dollySelectedKeyframe, 0, static_cast<int>(Menu::Camera::dollyKeyframes.size()) - 1);
	}

	int GetTimingModel() {
		return std::clamp(static_cast<int>(Menu::Camera::dollyTimingModel), kTimingModelTimeBased, kTimingModelArcLength);
	}

	float GetTurnSmoothingAggressiveness() {
		return std::clamp(static_cast<float>(Menu::Camera::dollyTurnSmoothingAggressiveness), 0.0f, 1.0f);
	}

	float GetCornerTangentScale() {
		return std::clamp(static_cast<float>(Menu::Camera::dollyCornerTangentScale), 0.0f, 2.5f);
	}

	uint64_t ComputeDollyKeyframeHash() {
		uint64_t hash = 1469598103934665603ull;
		const auto mix = [&hash](const float value) {
			const uint64_t component = static_cast<uint64_t>(std::hash<float>{}(value));
			hash ^= component + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
		};

		for (const auto& keyframe : Menu::Camera::dollyKeyframes) {
			mix(keyframe.time);
			mix(keyframe.position.X);
			mix(keyframe.position.Y);
			mix(keyframe.position.Z);
			mix(keyframe.rotation.X);
			mix(keyframe.rotation.Y);
			mix(keyframe.rotation.Z);
			mix(keyframe.fov);
		}
		return hash;
	}

	bool IsMeaningfulKeyframeChange(const Menu::Camera::DollyKeyframe& current, const Menu::Camera::DollyKeyframe& reference) {
		const vec3 deltaPos = current.position - reference.position;
		const vec3 deltaRot = current.rotation - reference.rotation;
		const float posDelta = std::sqrt(deltaPos.dot(deltaPos));
		const float rotDelta = std::max({ std::abs(deltaRot.X), std::abs(deltaRot.Y), std::abs(deltaRot.Z) });
		const float fovDelta = std::abs(current.fov - reference.fov);
		return posDelta >= std::max(0.001f, static_cast<float>(Menu::Camera::dollyRecordPositionThreshold)) ||
			rotDelta >= std::max(0.01f, static_cast<float>(Menu::Camera::dollyRecordRotationThreshold)) ||
			fovDelta >= std::max(0.01f, static_cast<float>(Menu::Camera::dollyRecordFOVThreshold));
	}

	void AddCurrentCameraAsKeyframeRuntime() {
		Menu::Camera::DollyKeyframe captured{};
		if (!CaptureCurrentDollyKeyframe(captured))
			return;
		const float interval = std::max(0.01f, static_cast<float>(Menu::Camera::dollyDefaultKeyframeInterval));
		captured.time = Menu::Camera::dollyKeyframes.empty() ? 0.0f : Menu::Camera::dollyKeyframes.back().time + interval;
		Menu::Camera::dollyKeyframes.push_back(captured);
		ClampTimelineState();
		Menu::Camera::dollySelectedKeyframe = static_cast<int>(Menu::Camera::dollyKeyframes.size()) - 1;
		Menu::Camera::dollyTimelineTime = captured.time;
	}

	void JumpToSelectedKeyframe(const int index) {
		if (Menu::Camera::dollyKeyframes.empty())
			return;
		Menu::Camera::dollySelectedKeyframe = std::clamp(index, 0, static_cast<int>(Menu::Camera::dollyKeyframes.size()) - 1);
		const auto& selected = Menu::Camera::dollyKeyframes[static_cast<size_t>(Menu::Camera::dollySelectedKeyframe)];
		Menu::Camera::dollyTimelineTime = selected.time;
		if (!Menu::Camera::dollyPlaying.GetValue()) {
			Menu::Camera::DollyKeyframe evaluated{};
			int upperHint = std::clamp(Menu::Camera::dollySelectedKeyframe + 1, 1, static_cast<int>(Menu::Camera::dollyKeyframes.size()) - 1);
			if (EvaluateDollyKeyframeAtTimeWithHint(selected.time, evaluated, upperHint))
				ApplyDollyKeyframe(evaluated);
			else
				ApplyDollyKeyframe(selected);
		}
	}

	void HandleRuntimeDollyActionHotkeys() {
		const bool menuClosed = !Menu::menuToggle.GetValue();
		if (!menuClosed)
			return;

		if (Menu::Camera::dollyAddKeyframe.IsKeyPressed()) {
			Menu::Camera::dollyAddKeyframe.SetIsKeyPressed(false);
			AddCurrentCameraAsKeyframeRuntime();
		}

		if (Menu::Camera::dollyPrevKeyframe.IsKeyPressed()) {
			Menu::Camera::dollyPrevKeyframe.SetIsKeyPressed(false);
			JumpToSelectedKeyframe(Menu::Camera::dollySelectedKeyframe - 1);
		}

		if (Menu::Camera::dollyNextKeyframe.IsKeyPressed()) {
			Menu::Camera::dollyNextKeyframe.SetIsKeyPressed(false);
			JumpToSelectedKeyframe(Menu::Camera::dollySelectedKeyframe + 1);
		}

		if (Menu::Camera::dollyClearPath.IsKeyPressed()) {
			Menu::Camera::dollyClearPath.SetIsKeyPressed(false);
			Menu::Camera::dollyKeyframes.clear();
			Menu::Camera::dollyPlaying.SetValue(false);
			Menu::Camera::dollyRecording.SetValue(false);
			Menu::Camera::dollyClearPathConfirm = false;
			ClampTimelineState();
		}
	}

	bool EvaluateDollyKeyframeAtRawTimeWithHint(const float rawTime, Menu::Camera::DollyKeyframe& outKeyframe, int& inOutUpperIndex) {
		auto& keyframes = Menu::Camera::dollyKeyframes;
		if (keyframes.empty())
			return false;
		if (keyframes.size() == 1) {
			outKeyframe = keyframes.front();
			outKeyframe.time = rawTime;
			return true;
		}

		const float clampedTime = std::clamp(rawTime, keyframes.front().time, keyframes.back().time);
		int upperIndex = std::clamp(inOutUpperIndex, 1, static_cast<int>(keyframes.size()) - 1);

		if (clampedTime < keyframes[static_cast<size_t>(upperIndex - 1)].time || clampedTime > keyframes[static_cast<size_t>(upperIndex)].time) {
			while (upperIndex < static_cast<int>(keyframes.size()) - 1 && clampedTime > keyframes[static_cast<size_t>(upperIndex)].time)
				upperIndex++;
			while (upperIndex > 1 && clampedTime < keyframes[static_cast<size_t>(upperIndex - 1)].time)
				upperIndex--;

			if (clampedTime < keyframes[static_cast<size_t>(upperIndex - 1)].time || clampedTime > keyframes[static_cast<size_t>(upperIndex)].time) {
				const auto upperIt = std::lower_bound(keyframes.begin(), keyframes.end(), clampedTime, [](const Menu::Camera::DollyKeyframe& kf, const float queryTime) {
					return kf.time < queryTime;
				});
				if (upperIt == keyframes.begin()) {
					outKeyframe = *upperIt;
					outKeyframe.time = clampedTime;
					inOutUpperIndex = 1;
					return true;
				}
				if (upperIt == keyframes.end()) {
					outKeyframe = keyframes.back();
					outKeyframe.time = clampedTime;
					inOutUpperIndex = static_cast<int>(keyframes.size()) - 1;
					return true;
				}
				upperIndex = static_cast<int>(std::distance(keyframes.begin(), upperIt));
			}
		}

		inOutUpperIndex = upperIndex;
		const int lowerIndex = upperIndex - 1;
		const auto& p1 = keyframes[static_cast<size_t>(lowerIndex)];
		const auto& p2 = keyframes[static_cast<size_t>(upperIndex)];
		const float segmentDuration = std::max(kMinTrackDuration, p2.time - p1.time);
		const float alpha = Clamp01((clampedTime - p1.time) / segmentDuration);
		const int p0Index = std::max(0, lowerIndex - 1);
		const int p3Index = std::min(static_cast<int>(keyframes.size()) - 1, upperIndex + 1);
		const auto& p0 = keyframes[static_cast<size_t>(p0Index)];
		const auto& p3 = keyframes[static_cast<size_t>(p3Index)];

		outKeyframe.time = clampedTime;
		if (static_cast<int>(Menu::Camera::dollyInterpolationCurve) == 1) {
			const float dt10 = SafeDelta(p0.time, p1.time);
			const float dt21 = SafeDelta(p1.time, p2.time);
			const float dt32 = SafeDelta(p2.time, p3.time);
			const float smoothingAggressiveness = GetTurnSmoothingAggressiveness();

			const vec3 guidePos0 = GetSoftenedGuidePosition(keyframes, p0Index, smoothingAggressiveness);
			const vec3 guidePos1 = GetSoftenedGuidePosition(keyframes, lowerIndex, smoothingAggressiveness);
			const vec3 guidePos2 = GetSoftenedGuidePosition(keyframes, upperIndex, smoothingAggressiveness);
			const vec3 guidePos3 = GetSoftenedGuidePosition(keyframes, p3Index, smoothingAggressiveness);

			const vec3 slopePos1 = ComputeMonotoneSlopeVec3(guidePos0, guidePos1, guidePos2, dt10, dt21);
			const vec3 slopePos2 = ComputeMonotoneSlopeVec3(guidePos1, guidePos2, guidePos3, dt21, dt32);
			const vec3 baseTangentPos1 = slopePos1 * dt21;
			const vec3 baseTangentPos2 = slopePos2 * dt21;
			const float spanPos1 = SafeDelta(p0.time, p2.time);
			const float spanPos2 = SafeDelta(p1.time, p3.time);
			const vec3 catmullLikeTangentPos1 = (guidePos2 - guidePos0) * (dt21 / spanPos1);
			const vec3 catmullLikeTangentPos2 = (guidePos3 - guidePos1) * (dt21 / spanPos2);
			vec3 tangentPos1 = LerpVec3(baseTangentPos1, catmullLikeTangentPos1, smoothingAggressiveness);
			vec3 tangentPos2 = LerpVec3(baseTangentPos2, catmullLikeTangentPos2, smoothingAggressiveness);
			const float cornerTangentScale = GetCornerTangentScale();
			tangentPos1 = tangentPos1 * cornerTangentScale;
			tangentPos2 = tangentPos2 * cornerTangentScale;
			outKeyframe.position = HermiteVec3(guidePos1, guidePos2, tangentPos1, tangentPos2, alpha);

			const float r0x = UnwrapAngleNear(p0.rotation.X, p1.rotation.X);
			const float r2x = UnwrapAngleNear(p2.rotation.X, p1.rotation.X);
			const float r3x = UnwrapAngleNear(p3.rotation.X, r2x);
			const float r0y = UnwrapAngleNear(p0.rotation.Y, p1.rotation.Y);
			const float r2y = UnwrapAngleNear(p2.rotation.Y, p1.rotation.Y);
			const float r3y = UnwrapAngleNear(p3.rotation.Y, r2y);
			const float r0z = UnwrapAngleNear(p0.rotation.Z, p1.rotation.Z);
			const float r2z = UnwrapAngleNear(p2.rotation.Z, p1.rotation.Z);
			const float r3z = UnwrapAngleNear(p3.rotation.Z, r2z);

			const float slopeRotX1 = ComputeMonotoneSlope(r0x, p1.rotation.X, r2x, dt10, dt21);
			const float slopeRotX2 = ComputeMonotoneSlope(p1.rotation.X, r2x, r3x, dt21, dt32);
			const float slopeRotY1 = ComputeMonotoneSlope(r0y, p1.rotation.Y, r2y, dt10, dt21);
			const float slopeRotY2 = ComputeMonotoneSlope(p1.rotation.Y, r2y, r3y, dt21, dt32);
			const float slopeRotZ1 = ComputeMonotoneSlope(r0z, p1.rotation.Z, r2z, dt10, dt21);
			const float slopeRotZ2 = ComputeMonotoneSlope(p1.rotation.Z, r2z, r3z, dt21, dt32);

			const float baseTangentRotX1 = slopeRotX1 * dt21;
			const float baseTangentRotX2 = slopeRotX2 * dt21;
			const float baseTangentRotY1 = slopeRotY1 * dt21;
			const float baseTangentRotY2 = slopeRotY2 * dt21;
			const float baseTangentRotZ1 = slopeRotZ1 * dt21;
			const float baseTangentRotZ2 = slopeRotZ2 * dt21;
			const float spanRot1 = SafeDelta(p0.time, p2.time);
			const float spanRot2 = SafeDelta(p1.time, p3.time);
			const float catmullTangentRotX1 = (r2x - r0x) * (dt21 / spanRot1);
			const float catmullTangentRotX2 = (r3x - p1.rotation.X) * (dt21 / spanRot2);
			const float catmullTangentRotY1 = (r2y - r0y) * (dt21 / spanRot1);
			const float catmullTangentRotY2 = (r3y - p1.rotation.Y) * (dt21 / spanRot2);
			const float catmullTangentRotZ1 = (r2z - r0z) * (dt21 / spanRot1);
			const float catmullTangentRotZ2 = (r3z - p1.rotation.Z) * (dt21 / spanRot2);
			const float tangentRotX1 = LerpFloat(baseTangentRotX1, catmullTangentRotX1, smoothingAggressiveness);
			const float tangentRotX2 = LerpFloat(baseTangentRotX2, catmullTangentRotX2, smoothingAggressiveness);
			const float tangentRotY1 = LerpFloat(baseTangentRotY1, catmullTangentRotY1, smoothingAggressiveness);
			const float tangentRotY2 = LerpFloat(baseTangentRotY2, catmullTangentRotY2, smoothingAggressiveness);
			const float tangentRotZ1 = LerpFloat(baseTangentRotZ1, catmullTangentRotZ1, smoothingAggressiveness);
			const float tangentRotZ2 = LerpFloat(baseTangentRotZ2, catmullTangentRotZ2, smoothingAggressiveness);
			const float smoothX = HermiteScalar(p1.rotation.X, r2x, tangentRotX1, tangentRotX2, alpha);
			const float smoothY = HermiteScalar(p1.rotation.Y, r2y, tangentRotY1, tangentRotY2, alpha);
			const float smoothZ = HermiteScalar(p1.rotation.Z, r2z, tangentRotZ1, tangentRotZ2, alpha);
			outKeyframe.rotation = vec3(
				NormalizeDegrees(smoothX),
				NormalizeDegrees(smoothY),
				NormalizeDegrees(smoothZ)
			);

			const float slopeFov1 = ComputeMonotoneSlope(p0.fov, p1.fov, p2.fov, dt10, dt21);
			const float slopeFov2 = ComputeMonotoneSlope(p1.fov, p2.fov, p3.fov, dt21, dt32);
			const float baseTangentFov1 = slopeFov1 * dt21;
			const float baseTangentFov2 = slopeFov2 * dt21;
			outKeyframe.fov = HermiteScalar(p1.fov, p2.fov, baseTangentFov1, baseTangentFov2, alpha);
		} else {
			outKeyframe.position = LerpVec3(p1.position, p2.position, alpha);
			outKeyframe.rotation = vec3(
				LerpAngleDegrees(p1.rotation.X, p2.rotation.X, alpha),
				LerpAngleDegrees(p1.rotation.Y, p2.rotation.Y, alpha),
				LerpAngleDegrees(p1.rotation.Z, p2.rotation.Z, alpha)
			);
			outKeyframe.fov = p1.fov + (p2.fov - p1.fov) * alpha;
		}

		return true;
	}

	bool RebuildArcLengthCache() {
		auto& keyframes = Menu::Camera::dollyKeyframes;
		arcLengthCache.samples.clear();
		arcLengthCache.totalDistance = 0.0f;
		arcLengthCache.valid = false;
		if (keyframes.size() < 2)
			return false;

		const float firstTime = keyframes.front().time;
		const float lastTime = keyframes.back().time;
		const float duration = std::max(kMinTrackDuration, lastTime - firstTime);
		const int sampleCount = std::clamp(static_cast<int>(keyframes.size() * 24), kArcLengthSampleMin, kArcLengthSampleMax);
		arcLengthCache.samples.reserve(static_cast<size_t>(sampleCount + 1));

		int upperIndexHint = 1;
		Menu::Camera::DollyKeyframe previous{};
		bool hasPrevious = false;
		float cumulativeDistance = 0.0f;
		for (int i = 0; i <= sampleCount; i++) {
			const float t = firstTime + duration * (static_cast<float>(i) / static_cast<float>(sampleCount));
			Menu::Camera::DollyKeyframe evaluated{};
			if (!EvaluateDollyKeyframeAtRawTimeWithHint(t, evaluated, upperIndexHint))
				continue;

			if (hasPrevious) {
				const vec3 delta = evaluated.position - previous.position;
				cumulativeDistance += std::sqrt(delta.dot(delta));
			}

			arcLengthCache.samples.push_back(ArcLengthSample{
				.distance = cumulativeDistance,
				.rawTime = t
			});
			previous = evaluated;
			hasPrevious = true;
		}

		arcLengthCache.totalDistance = cumulativeDistance;
		arcLengthCache.keyHash = ComputeDollyKeyframeHash();
		arcLengthCache.interpolationCurve = static_cast<int>(Menu::Camera::dollyInterpolationCurve);
		arcLengthCache.smoothingAggressiveness = GetTurnSmoothingAggressiveness();
		arcLengthCache.turnSmoothingDeviateKeyframes = Menu::Camera::dollyTurnSmoothingDeviateKeyframes.GetValue();
		arcLengthCache.cornerTangentScale = GetCornerTangentScale();
		arcLengthCache.valid = arcLengthCache.samples.size() >= 2;
		return arcLengthCache.valid;
	}

	bool EnsureArcLengthCache() {
		if (GetTimingModel() != kTimingModelArcLength)
			return false;
		if (Menu::Camera::dollyKeyframes.size() < 2)
			return false;

		const uint64_t keyHash = ComputeDollyKeyframeHash();
		const int interpolationCurve = static_cast<int>(Menu::Camera::dollyInterpolationCurve);
		const float smoothingAggressiveness = GetTurnSmoothingAggressiveness();
		const bool deviateKeyframes = Menu::Camera::dollyTurnSmoothingDeviateKeyframes.GetValue();
		const float cornerTangentScale = GetCornerTangentScale();
		if (!arcLengthCache.valid ||
			arcLengthCache.keyHash != keyHash ||
			arcLengthCache.interpolationCurve != interpolationCurve ||
			!EGSDK::Utils::Values::are_samef(arcLengthCache.smoothingAggressiveness, smoothingAggressiveness) ||
			arcLengthCache.turnSmoothingDeviateKeyframes != deviateKeyframes ||
			!EGSDK::Utils::Values::are_samef(arcLengthCache.cornerTangentScale, cornerTangentScale)) {
			return RebuildArcLengthCache();
		}
		return true;
	}

	float MapTimelineTimeToRawTime(const float timelineTime) {
		auto& keyframes = Menu::Camera::dollyKeyframes;
		if (keyframes.empty())
			return 0.0f;
		const float clampedTime = std::clamp(timelineTime, keyframes.front().time, keyframes.back().time);
		if (GetTimingModel() != kTimingModelArcLength || !EnsureArcLengthCache())
			return clampedTime;
		if (arcLengthCache.samples.size() < 2 || arcLengthCache.totalDistance <= kEpsilon)
			return clampedTime;

		const float firstTime = keyframes.front().time;
		const float duration = std::max(kMinTrackDuration, keyframes.back().time - firstTime);
		const float normalizedTime = Clamp01((clampedTime - firstTime) / duration);
		const float targetDistance = normalizedTime * arcLengthCache.totalDistance;

		const auto& samples = arcLengthCache.samples;
		const auto upperIt = std::lower_bound(samples.begin(), samples.end(), targetDistance, [](const ArcLengthSample& sample, const float queryDistance) {
			return sample.distance < queryDistance;
		});
		if (upperIt == samples.begin())
			return upperIt->rawTime;
		if (upperIt == samples.end())
			return samples.back().rawTime;

		const auto lowerIt = upperIt - 1;
		const float distanceSpan = std::max(kMinTrackDuration, upperIt->distance - lowerIt->distance);
		const float alpha = Clamp01((targetDistance - lowerIt->distance) / distanceSpan);
		return LerpFloat(lowerIt->rawTime, upperIt->rawTime, alpha);
	}

	bool CaptureCurrentDollyKeyframe(Menu::Camera::DollyKeyframe& outKeyframe) {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return false;
		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return false;

		vec3 camPos{};
		vec3 camForward{};
		vec3 camUp{};
		if (!viewCam->GetPosition(&camPos) || !viewCam->GetForwardVector(&camForward) || !viewCam->GetUpVector(&camUp))
			return false;

		outKeyframe.position = camPos;
		outKeyframe.rotation = EulerFromVectors(camForward, camUp);
		outKeyframe.fov = viewCam->GetFOV();
		return true;
	}

	bool ApplyDollyKeyframe(const Menu::Camera::DollyKeyframe& keyframe) {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return false;
		auto* viewCam = iLevel->GetViewCamera();
		if (!viewCam)
			return false;

		const mtx34 invCameraMtx = BuildInvCameraMatrix(keyframe.position, keyframe.rotation);
		const mtx34 viewCameraMtx = BuildViewCameraMatrix(keyframe.position, keyframe.rotation);
		viewCam->SetInvCameraMatrix(&invCameraMtx);
		viewCam->SetCameraMatrix(&viewCameraMtx);
		viewCam->SetPosition(&keyframe.position);
		viewCam->SetFOV(keyframe.fov);
		return true;
	}

	bool EvaluateDollyKeyframeAtTimeWithHint(const float timelineTime, Menu::Camera::DollyKeyframe& outKeyframe, int& inOutUpperIndex) {
		const float rawTime = MapTimelineTimeToRawTime(timelineTime);
		return EvaluateDollyKeyframeAtRawTimeWithHint(rawTime, outKeyframe, inOutUpperIndex);
	}

	bool EvaluateDollyKeyframeAtTime(const float timelineTime, Menu::Camera::DollyKeyframe& outKeyframe) {
		int upperIndexHint = 1;
		return EvaluateDollyKeyframeAtTimeWithHint(timelineTime, outKeyframe, upperIndexHint);
	}

	void UpdateCameraDollyRuntime() {
		if (IsDollyRuntimeUnavailable()) {
			Menu::Camera::dollyPlaying.SetValue(false);
			Menu::Camera::dollyRecording.SetValue(false);
			return;
		}
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded()) {
			Menu::Camera::dollyPlaying.SetValue(false);
			Menu::Camera::dollyRecording.SetValue(false);
			return;
		}

		ClampTimelineState();
		HandleRuntimeDollyActionHotkeys();
		ClampTimelineState();
		auto& keyframes = Menu::Camera::dollyKeyframes;

		static bool prevRecording = false;
		static float recordingAccumulator = 0.0f;
		static float recordingSessionTime = 0.0f;
		static float recordingStartOffset = 0.0f;
		static Menu::Camera::DollyKeyframe lastRecordedKeyframe{};
		static bool hasLastRecordedKeyframe = false;
		const float deltaTime = std::max(0.0f, ImGui::GetIO().DeltaTime);

		if (!prevRecording && Menu::Camera::dollyRecording.GetValue()) {
			recordingAccumulator = 0.0f;
			recordingSessionTime = 0.0f;
			recordingStartOffset = Menu::Camera::dollyDuration;
			hasLastRecordedKeyframe = false;

			Menu::Camera::DollyKeyframe initialRecorded{};
			if (CaptureCurrentDollyKeyframe(initialRecorded)) {
				initialRecorded.time = recordingStartOffset;
				keyframes.push_back(initialRecorded);
				lastRecordedKeyframe = initialRecorded;
				hasLastRecordedKeyframe = true;
				Menu::Camera::dollySelectedKeyframe = static_cast<int>(keyframes.size()) - 1;
				Menu::Camera::dollyTimelineTime = initialRecorded.time;
				ClampTimelineState();
			}
		}

		if (Menu::Camera::dollyRecording.GetValue()) {
			Menu::Camera::DollyKeyframe currentSample{};
			if (CaptureCurrentDollyKeyframe(currentSample)) {
				const bool meaningful = !hasLastRecordedKeyframe || IsMeaningfulKeyframeChange(currentSample, lastRecordedKeyframe);
				const bool pauseWhenIdle = Menu::Camera::dollyPauseRecordingTimeWhenIdle.GetValue();

				if (!pauseWhenIdle || meaningful)
					recordingSessionTime += deltaTime;
				if (!pauseWhenIdle || meaningful)
					recordingAccumulator += deltaTime;

				if (meaningful && recordingAccumulator >= std::max(0.05f, static_cast<float>(Menu::Camera::dollyRecordMinInterval))) {
					recordingAccumulator = 0.0f;
					currentSample.time = recordingStartOffset + recordingSessionTime;
					keyframes.push_back(currentSample);
					lastRecordedKeyframe = currentSample;
					hasLastRecordedKeyframe = true;

					ClampTimelineState();
					Menu::Camera::dollySelectedKeyframe = static_cast<int>(keyframes.size()) - 1;
					Menu::Camera::dollyTimelineTime = keyframes.back().time;
				}
			}
		} else {
			recordingAccumulator = 0.0f;
			recordingSessionTime = 0.0f;
			recordingStartOffset = 0.0f;
			hasLastRecordedKeyframe = false;
		}

		if (keyframes.empty()) {
			Menu::Camera::dollyPlaying.SetValue(false);
			prevRecording = Menu::Camera::dollyRecording.GetValue();
			return;
		}

		if (Menu::Camera::dollyPlayPause.IsKeyPressed()) {
			Menu::Camera::dollyPlayPause.SetIsKeyPressed(false);
			Menu::Camera::dollyPlaying.Toggle();
		}
		if (Menu::Camera::dollyPlaying.GetValue()) {
			const bool gamePaused = iLevel->IsTimerFrozen();
			if (!gamePaused) {
				Menu::Camera::dollyTimelineTime += deltaTime * std::max(0.01f, static_cast<float>(Menu::Camera::dollyPlaybackSpeed));
				if (Menu::Camera::dollyTimelineTime >= Menu::Camera::dollyDuration) {
					if (Menu::Camera::dollyLoop.GetValue() && Menu::Camera::dollyDuration > kMinTrackDuration)
						Menu::Camera::dollyTimelineTime = 0.0f;
					else {
						Menu::Camera::dollyTimelineTime = Menu::Camera::dollyDuration;
						Menu::Camera::dollyPlaying.SetValue(false);
					}
				}
			}

			Menu::Camera::DollyKeyframe evaluated{};
			if (EvaluateDollyKeyframeAtTime(Menu::Camera::dollyTimelineTime, evaluated))
				ApplyDollyKeyframe(evaluated);
		}
		prevRecording = Menu::Camera::dollyRecording.GetValue();
	}
}
