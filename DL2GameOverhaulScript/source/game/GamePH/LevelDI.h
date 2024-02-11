#pragma once
#include <WTypesbase.h>

namespace GamePH {
	namespace TimeWeather {
		class CSystem;
	}

	class LevelDI {
	public:
		bool IsLoading();
		bool IsLoaded();
		LPVOID GetViewCamera();
		float GetTimeDelta();
		void SetViewCamera(LPVOID viewCam);
		float GetTimePlayed();
		void ShowUIManager(bool enabled);
		float TimerGetSpeedUp();
		void TimerSetSpeedUp(float timeScale);
		TimeWeather::CSystem* GetTimeWeatherSystem();

		static LevelDI* Get();
	};
}