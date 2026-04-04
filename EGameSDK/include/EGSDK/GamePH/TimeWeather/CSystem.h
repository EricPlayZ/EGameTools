#pragma once
#include <EGSDK\GamePH\TimeWeather\ISubsystem.h>
#include <EGSDK\GamePH\TimeWeather\EWeather.h>
#include <EGSDK\Exports.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	namespace TimeWeather {
		class EGameSDK_API CSystem {
		public:
			union {
				DynamicField(CSystem, float, blendTime);
				DynamicField(CSystem, float, blendTime2);
				DynamicField(CSystem, ISubsystem*, currentSubSystem);
				DynamicField(CSystem, ISubsystem*, nextSubSystem);
				DynamicField(CSystem, ISubsystem*, lastSubSystem);
			};

			void SetForcedWeather(int weather);
			void ClearForcedWeather();
			// ?GetCurrentWeather@CSystem@TimeWeather@@QEBA?AW4TYPE@EWeather@@XZ
			EWeather GetCurrentWeather() const;

			void ReloadSubsystems();
			void RequestTimeWeatherInterpolation(int weather, float a3, float a4);
			void FinishTimeWeatherInterpolation();
			bool IsFullyBlended();

			static CSystem* Get();
		};
	}
}