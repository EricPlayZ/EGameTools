#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <any>
#include "hook.h"

#define STR_MERGE_IMPL(a, b) a##b
#define STR_MERGE(a, b) STR_MERGE_IMPL(a, b)
#define MAKE_PAD(size) STR_MERGE(_pad, __COUNTER__)[size]
#define DEFINE_MEMBER_N(type, name, offset) struct {unsigned char MAKE_PAD(offset); type name;}
#define DEFINE_MEMBER_0(type, name) struct {type name;}

// Game structs
struct Vector3 {
	float X;
	float Y;
	float Z;

	Vector3() {
		X = 0.0f;
		Y = 0.0f;
		Z = 0.0f;
	}

	bool isDefault() {
		return X == 0.0f && Y == 0.0f && Z == 0.0f;
	}
};
namespace EWeather {
	enum TYPE {
		Default = 0,
		Foggy = 1,
		Clear = 2,
		Overcast = 3,
		Cloudy = 4,
		Rainy = 5,
		Stormy = 6
	};
}

// Forward decl
namespace Engine {
	class CoPhysicsProperty;
}

namespace GamePH {
	extern void LoopHookCreatePlayerHealthModule();
	extern void LoopHookOnUpdate();
	extern void LoopHookCalculateFreeCamCollision();
	extern void LoopHookLifeSetHealth();
	extern void LoopHookTogglePhotoMode();

	class PlayerVariables {
	public:
		static std::vector<std::pair<std::string, std::pair<LPVOID, std::string>>> playerVars;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerVarsDefault;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerCustomVarsDefault;
		static bool gotPlayerVars;

		static void SortPlayerVars();

		static PDWORD64 GetFloatPlayerVariableVT();
		static PDWORD64 GetBoolPlayerVariableVT();
		static void GetPlayerVars();

		static PlayerVariables* Get();
	};

	class PlayerState {
	public:
		union {
			DEFINE_MEMBER_N(PlayerVariables*, playerVars, 0x290);
		};

		static PlayerState* Get();
	};

	class PlayerHealthModule {
	public:
		union {
			DEFINE_MEMBER_N(float, health, 0x2C);
		};

		static PlayerHealthModule* pPlayerHealthModule;
		static PlayerHealthModule* Get();
	};

	class CameraFPPDI {
	public:
		static CameraFPPDI* Get();
	};

	class FreeCamera {
	public:
		Vector3* GetPosition(Vector3* posIN);
		void AllowCameraMovement(int mode = 2);

		static FreeCamera* Get();
	};

	class DayNightCycle {
	public:
		union {
			DEFINE_MEMBER_N(float, time1, 0x10);
			DEFINE_MEMBER_N(float, time2, 0x20);
			DEFINE_MEMBER_N(float, time3, 0x5C);
		};

		void SetDaytime(float time);

		static DayNightCycle* Get();
	};

	namespace TimeWeather {
		class CSystem {
		public:
			void SetForcedWeather(int weather);
			int GetCurrentWeather();

			static CSystem* Get();
		};
	}

	class LevelDI {
	public:
		float GetTimePlayed();
		LPVOID GetViewCamera();
		TimeWeather::CSystem* GetTimeWeatherSystem();

		static LevelDI* Get();
	};

	class GameDI_PH2 {
	public:
		static GameDI_PH2* Get();
	};

	class GameDI_PH {
	public:
		INT64 GetCurrentGameVersion();
		void TogglePhotoMode(bool doNothing = false, bool setAsOptionalCamera = false);

		static GameDI_PH* Get();
	};

	class PlayerObjProperties {
	public:
		union {
			DEFINE_MEMBER_N(Engine::CoPhysicsProperty*, pCoPhysicsProperty, 0xF0);
		};

		static PlayerObjProperties* Get();
	};
}

namespace Engine {
	class CVideoSettings {
	public:
		union {
			DEFINE_MEMBER_N(float, ExtraFOV, 0x7C);
		};

		static CVideoSettings* Get();
	};

	class CLevel {
	public:
		union {
			DEFINE_MEMBER_N(GamePH::LevelDI*, pLevelDI, 0x20);
		};

		static CLevel* Get();
	};

	class CGame {
	public:
		union {
			DEFINE_MEMBER_N(GamePH::GameDI_PH*, pGameDI_PH, 0x8);
			DEFINE_MEMBER_N(CVideoSettings*, pCVideoSettings, 0x28);
			DEFINE_MEMBER_N(CLevel*, pCLevel, 0x380);
		};

		static CGame* Get();
	};

	class CLobbySteam {
	public:
		union {
			DEFINE_MEMBER_N(CGame*, pCGame, 0xF8);
		};

		static CLobbySteam* Get();
	};

	class CInput {
	public:
		DWORD64 BlockGameInput();
		void UnlockGameInput();

		static CInput* Get();
	};

	class CBulletPhysicsCharacter {
	public:
		union {
			DEFINE_MEMBER_N(Vector3, playerPos2, 0x8A0);
			DEFINE_MEMBER_N(Vector3, playerPos, 0x8B8);
			DEFINE_MEMBER_N(float, playerDownwardVelocity, 0xC38);
		};

		static Vector3 posBeforeFreeze;

		void FreezeCharacter();
		void MoveCharacter(const Vector3& pos);

		static CBulletPhysicsCharacter* Get();
	};

	class CoPhysicsProperty {
	public:
		union {
			DEFINE_MEMBER_N(CBulletPhysicsCharacter*, pCBulletPhysicsCharacter, 0x20);
		};

		static CoPhysicsProperty* Get();
	};
}

#undef DEFINE_MEMBER_0
#undef DEFINE_MEMBER_N
#undef MAKE_PAD
#undef STR_MERGE
#undef STR_MERGE_IMPL