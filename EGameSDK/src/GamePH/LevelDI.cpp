#include <EGSDK\Offsets.h>
#include <EGSDK\Engine\CLevel.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\WinMemory.h>
#include <EGSDK\Utils\Time.h>

namespace EGSDK::GamePH {
	Utils::Time::Timer LevelDI::loadTimer{ 20000 };
	bool LevelDI::relyOnTimer = false;
	bool LevelDI::hasLoaded = false;

	bool LevelDI::IsLoading() {
		return Utils::Memory::SafeCallFunction<bool>("engine_x64_rwdi.dll", "?IsLoading@ILevel@@QEBA_NXZ", true, this);
	}
	bool LevelDI::IsLoaded() {
		if (IsLoading() || !GamePH::PlayerDI_PH::Get()) {
			ResetLoadTimer();
			return false;
		}

		if (!relyOnTimer && !hasLoaded) {
			relyOnTimer = true;
			loadTimer.Reset();
			return false;
		}

		if (loadTimer.DidTimePass()) {
			relyOnTimer = false;
			hasLoaded = true;
			return true;
		}

		return false;
	}
	void LevelDI::ResetLoadTimer() {
		relyOnTimer = false;
		hasLoaded = false;
	}
	void* LevelDI::GetCurrentRegionName() {
		return Utils::Memory::SafeCallFunction<void*>("engine_x64_rwdi.dll", "?GetCurrentRegionName@ILevel@@UEBAAEBV?$string_base@D@ttl@@XZ", nullptr, this);
	}
	const char* LevelDI::GetLevelName() {
		return Utils::Memory::SafeCallFunction<const char*>("engine_x64_rwdi.dll", "?GetLevelName@ILevel@@UEBAPEBDXZ", nullptr, this);
	}
	Engine::IBaseCamera* LevelDI::GetViewCamera() {
		return Utils::Memory::SafeCallFunction<Engine::IBaseCamera*>("engine_x64_rwdi.dll", "?GetViewCamera@ILevel@@QEBAPEAVIBaseCamera@@XZ", nullptr, this);
	}
	float LevelDI::GetTimeDelta() {
		return Utils::Memory::SafeCallFunction<float>("gamedll_ph_x64_rwdi.dll", "?GetTimeDelta@IGSObject@@UEBAMXZ", 0.0f, this);
	}
	void LevelDI::SetViewCamera(void* viewCam) {
		Utils::Memory::SafeCallFunctionVoid("gamedll_ph_x64_rwdi.dll", "?SetViewCamera@ILevel@@UEAAXPEAVIBaseCamera@@@Z_0", this, viewCam);
	}
	float LevelDI::GetTimePlayed() {
		return Utils::Memory::SafeCallFunction<float>("gamedll_ph_x64_rwdi.dll", "?GetTimePlayed@ILevel@@UEBAMXZ", 0.0f, this);
	}
	void LevelDI::ShowUIManager(bool enabled) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?ShowUIManager@ILevel@@QEAAX_N@Z", this, enabled);
	}
	bool LevelDI::IsTimerFrozen() {
		return Utils::Memory::SafeCallFunction<bool>("engine_x64_rwdi.dll", "?IsTimerFrozen@ILevel@@QEBA_NXZ", false, this);
	}
	float LevelDI::TimerGetSpeedUp() {
		return Utils::Memory::SafeCallFunction<float>("engine_x64_rwdi.dll", "?TimerGetSpeedUp@ILevel@@QEBAMXZ", -1.0f, this);
	}
	void LevelDI::TimerSetSpeedUp(float timeScale) {
		Utils::Memory::SafeCallFunctionVoid("engine_x64_rwdi.dll", "?TimerSetSpeedUp@ILevel@@QEAAXM@Z", this, timeScale);
	}
	TimeWeather::CSystem* LevelDI::GetTimeWeatherSystem() {
		return Utils::Memory::SafeCallFunction<TimeWeather::CSystem*>("engine_x64_rwdi.dll", "?GetTimeWeatherSystem@ILevel@@QEBAPEAVCSystem@TimeWeather@@XZ", nullptr, this);
	}

	static LevelDI* GetOffset_LevelDI() {
		Engine::CLevel* pCLevel = Engine::CLevel::Get();
		return pCLevel ? pCLevel->pLevelDI : nullptr;
	}
	LevelDI* LevelDI::Get() {
		LevelDI* ptr = ClassHelpers::SafeGetter<LevelDI>(GetOffset_LevelDI, false);
		if (!ptr)
			ResetLoadTimer();

		return ptr;
	}
}