#pragma once
#include <WTypesbase.h>
#include "..\buffer.h"
#include "..\utils\time.h"

namespace GamePH {
	namespace TimeWeather {
		class CSystem;
	}

	class PlayerDI_PH;

	class LevelDI {
	public:
		bool IsLoading();
		bool IsLoaded();
		LPVOID GetViewCamera();
		float GetTimeDelta();
		void SetViewCamera(LPVOID viewCam);
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