#include <pch.h>

namespace Engine {
	namespace GameSpeedHandler {
		float speed = 1.0;
		bool initialized = false;

/*#pragma region GetTickCount64Hook
		ULONGLONG GTC64_BaseTime = 0;
		ULONGLONG GTC64_OffsetTime = 0;

		static LPVOID GetGetTickCount64() {
			return &GetTickCount64;
		}
		static ULONGLONG WINAPI detourGetTickCount64();
		static Utils::Hook::MHook<LPVOID, ULONGLONG(WINAPI*)()> GetTickCount64Hook{ "GetTickCount64", &GetGetTickCount64, &detourGetTickCount64 };

		ULONGLONG WINAPI detourGetTickCount64() {
			return GTC64_OffsetTime + ((GetTickCount64Hook.pOriginal() - GTC64_BaseTime) * static_cast<double>(speed));
		}
#pragma endregion

#pragma region QueryPerformanceCounterHook
		LARGE_INTEGER QPC_BaseTime = LARGE_INTEGER();
		LARGE_INTEGER QPC_OffsetTime = LARGE_INTEGER();

		static LPVOID GetQueryPerformanceCounter() {
			return &QueryPerformanceCounter;
		}
		static BOOL WINAPI detourQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
		static Utils::Hook::MHook<LPVOID, BOOL(WINAPI*)(LARGE_INTEGER*)> QueryPerformanceCounterHook{ "QueryPerformanceCounter", &GetQueryPerformanceCounter, &detourQueryPerformanceCounter };

		BOOL WINAPI detourQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
			LARGE_INTEGER x;
			QueryPerformanceCounterHook.pOriginal(&x);
			lpPerformanceCount->QuadPart = QPC_OffsetTime.QuadPart + ((x.QuadPart - QPC_BaseTime.QuadPart) * static_cast<double>(speed));

			return true;
		}
#pragma endregion

		void Setup() {
			if (!GetTickCount64Hook.pOriginal || !QueryPerformanceCounterHook.pOriginal || initialized)
				return;

			GTC64_BaseTime = GetTickCount64Hook.pOriginal();
			GTC64_OffsetTime = GTC64_BaseTime;

			QueryPerformanceCounterHook.pOriginal(&QPC_BaseTime);
			QPC_OffsetTime.QuadPart = QPC_BaseTime.QuadPart;

			initialized = true;
		}

		void SetGameSpeed(float gameSpeed) {
			if (initialized) {
				GTC64_OffsetTime = detourGetTickCount64();
				GTC64_BaseTime = GetTickCount64Hook.pOriginal();

				detourQueryPerformanceCounter(&QPC_OffsetTime);
				QueryPerformanceCounterHook.pOriginal(&QPC_BaseTime);
			}

			speed = gameSpeed;
		}*/
	}
}