#include <ImGui\imguiex_animation.h>
#include <EGSDK\GamePH\DayNightCycle.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\World\WorldRuntime_Internal.h>
#include <EGT\Menu\World.h>

namespace EGT::GamePH::World {
	static float timeBeforeFreeze = 0.0f;
	static bool isModifyingGameSpeed = false;
	static float actualGameSpeed = Menu::World::gameSpeed;
	static float gameSpeedBeforeSlowMo = Menu::World::gameSpeed;
	static float slowMotionSpeedLerp = Menu::World::gameSpeed;
	static float gameSpeedBeforeFreezeGameSpeed = Menu::World::gameSpeed;

	static void UpdateFreezeTime() {
		auto* dayNightCycle = EGSDK::GamePH::DayNightCycle::Get();
		if (!dayNightCycle)
			return;
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;

		if (Menu::World::freezeTime.HasChangedTo(true)) {
			timeBeforeFreeze = Menu::World::time;
			Menu::World::freezeTime.SetPrevValue(true);
		} else if (Menu::World::freezeTime.HasChangedTo(false)) {
			dayNightCycle->SetDaytime(timeBeforeFreeze);
			Menu::World::freezeTime.SetPrevValue(false);
		}

		Menu::World::time = dayNightCycle->time1 * 24.0f;
		if (Menu::World::freezeTime.GetValue() && !EGSDK::Utils::Values::are_samef(Menu::World::time, timeBeforeFreeze, 0.0095f))
			dayNightCycle->SetDaytime(timeBeforeFreeze);
	}

	static void UpdateSlowMo() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return;

		static bool slowMoHasChanged = true;
		static bool freezeGameSpeedActiveLastFrame = false;
		constexpr float kFreezeGameSpeedStart = 0.0001f;
		constexpr float kFreezeGameSpeedTarget = 0.00001f;
		constexpr float kFreezeGameSpeedLerpSeconds = 1.0f;

		if (Menu::World::slowMotion.GetValue() && Menu::World::freezeGameSpeed.GetValue())
			Menu::World::slowMotion.SetValue(false);

		const bool freezeGameSpeedJustEnabled = Menu::World::freezeGameSpeed.HasChangedTo(true);
		if (freezeGameSpeedJustEnabled) {
			gameSpeedBeforeFreezeGameSpeed = actualGameSpeed;
			Menu::World::freezeGameSpeed.SetPrevValue(true);
		}

		if (Menu::World::freezeGameSpeed.GetValue()) {
			const float freezeSpeedLerp = ImGui::AnimateLerp("freezeGameSpeedLerp", kFreezeGameSpeedStart, kFreezeGameSpeedTarget, kFreezeGameSpeedLerpSeconds, freezeGameSpeedJustEnabled);
			iLevel->TimerSetSpeedUp(freezeSpeedLerp);
			actualGameSpeed = iLevel->TimerGetSpeedUp();
			freezeGameSpeedActiveLastFrame = true;
			return;
		}

		if (freezeGameSpeedActiveLastFrame || Menu::World::freezeGameSpeed.HasChangedTo(false)) {
			iLevel->TimerSetSpeedUp(gameSpeedBeforeFreezeGameSpeed);
			actualGameSpeed = iLevel->TimerGetSpeedUp();
			freezeGameSpeedActiveLastFrame = false;
			Menu::World::freezeGameSpeed.SetPrevValue(false);
		}

		if (Menu::World::slowMotion.HasChangedTo(false)) {
			static float gameSpeedAfterChange = 0.0f;
			if (slowMoHasChanged)
				gameSpeedAfterChange = actualGameSpeed;

			slowMotionSpeedLerp = ImGui::AnimateLerp("slowMotionSpeedLerp", gameSpeedAfterChange, gameSpeedBeforeSlowMo, Menu::World::slowMotionTransitionTime, slowMoHasChanged, &ImGui::AnimEaseInOutSine);
			iLevel->TimerSetSpeedUp(slowMotionSpeedLerp);
			slowMoHasChanged = false;

			if (EGSDK::Utils::Values::are_samef(actualGameSpeed, gameSpeedBeforeSlowMo)) {
				slowMoHasChanged = true;
				Menu::World::slowMotion.SetPrevValue(false);
			}
		} else if (Menu::World::slowMotion.GetValue()) {
			static float gameSpeedAfterChange = 0.0f;
			if (Menu::World::slowMotion.HasChanged()) {
				if (slowMoHasChanged)
					gameSpeedBeforeSlowMo = actualGameSpeed;
				gameSpeedAfterChange = actualGameSpeed;
			}

			slowMotionSpeedLerp = ImGui::AnimateLerp("slowMotionSpeedLerp", gameSpeedAfterChange, Menu::World::slowMotionSpeed, Menu::World::slowMotionTransitionTime, Menu::World::slowMotion.HasChanged(), &ImGui::AnimEaseInOutSine);
			iLevel->TimerSetSpeedUp(slowMotionSpeedLerp);

			if (Menu::World::slowMotion.HasChanged()) {
				slowMoHasChanged = true;
				Menu::World::slowMotion.SetPrevValue(Menu::World::slowMotion.GetValue());
			}
		}

		if (!isModifyingGameSpeed) {
			if (!Menu::World::slowMotion.GetValue() && !Menu::World::slowMotion.HasChanged() && !EGSDK::Utils::Values::are_samef(Menu::World::gameSpeed, 1.0f))
				iLevel->TimerSetSpeedUp(Menu::World::gameSpeed);
			actualGameSpeed = iLevel->TimerGetSpeedUp();
		}
	}

	void OnWorldTimeSliderChanged(float timeValue) {
		Menu::World::time = timeValue;
		timeBeforeFreeze = timeValue;
	}

	void SetWorldIsModifyingGameSpeed(bool value) {
		isModifyingGameSpeed = value;
	}

	void UpdateWorldTimeRuntime() {
		UpdateFreezeTime();
		UpdateSlowMo();
	}
}
