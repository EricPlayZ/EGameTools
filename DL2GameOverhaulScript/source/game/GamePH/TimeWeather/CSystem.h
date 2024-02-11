#pragma once
namespace GamePH {
	namespace TimeWeather {
		class CSystem {
		public:
			void SetForcedWeather(int weather);
			int GetCurrentWeather();

			static CSystem* Get();
		};
	}
}