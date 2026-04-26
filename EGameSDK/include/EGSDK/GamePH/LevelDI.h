#pragma once
#include <EGSDK\Engine\IBaseCamera.h>
#include <EGSDK\GamePH\TimeWeather\CSystem.h>
#include <EGSDK\GamePH\AIManager.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Utils\Time.h>

namespace EGSDK::GamePH {
	class EGameSDK_API LevelDI {
	public:
		union {
			DynamicField(LevelDI, AIManager*, pBaseAIManager);
		};

		bool IsLoading();
		bool IsLoaded();
		void* GetCurrentRegionName();
		const char* GetLevelName();
		Engine::IBaseCamera* GetViewCamera();
		float GetTimeDelta();
		void SetViewCamera(void* viewCam);
		float GetTimePlayed();
		void ShowUIManager(bool enabled);
		bool IsTimerFrozen();
		float TimerGetSpeedUp();
		void TimerSetSpeedUp(float timeScale);
		TimeWeather::CSystem* GetTimeWeatherSystem();

		static LevelDI* Get();
	private:
		static Utils::Time::Timer loadTimer;
		static bool relyOnTimer;
		static bool hasLoaded;

		static void ResetLoadTimer();
	};
}