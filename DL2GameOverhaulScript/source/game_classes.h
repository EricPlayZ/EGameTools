#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <string_view>
#include <memory>
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
};

// Forward decl
namespace Engine {
	class CoPhysicsProperty;
}

namespace GamePH {
	extern void LoopHookOnUpdate();
	extern void LoopHookCalculateFreeCamCollision();

	class PlayerVariables {
	private:
		static DWORD64 FloatPlayerVariableVT;
		static DWORD64 BoolPlayerVariableVT;
	public:
		static std::vector<std::pair<std::string_view, std::pair<DWORD64, std::string_view>>> unorderedPlayerVars;
		static bool gotPlayerVars;

		static std::unique_ptr<Hook::BreakpointHook> loadPlayerFloatVarBpHook;
		static std::unique_ptr<Hook::BreakpointHook> loadPlayerBoolVarBpHook;

		static bool hooked;
		static void RunHooks();

		static DWORD64 GetFloatPlayerVariableVT();
		static DWORD64 GetBoolPlayerVariableVT();
		static void GetPlayerVars();

		static PlayerVariables* Get();
	};

	class PlayerState {
	public:
		union {
			DEFINE_MEMBER_N(PlayerVariables*, playerVars, 0x278);
		};

		static PlayerState* Get();
	};

	class FreeCamera {
	public:
		void AllowCameraMovement(int mode = 2);

		static FreeCamera* Get();
	};

	class LevelDI {
	public:
		float GetTimePlayed();

		static LevelDI* Get();
	};

	class GameDI_PH2 {
	public:
		static GameDI_PH2* Get();
	};

	class GameDI_PH {
	public:
		DWORD64* GetLocalPlayerEntity();
		INT64 GetCurrentGameVersion();
		void TogglePhotoMode(bool doNothing = false, bool setAsOptionalCamera = false);

		static GameDI_PH* Get();
	};

	class PlayerObjProperties {
	public:
		union {
			DEFINE_MEMBER_N(Engine::CoPhysicsProperty*, pCoPhysicsProperty, 0xE8);
		};

		static PlayerObjProperties* Get();
	};
}

namespace Engine {
	class CVideoSettings {
	public:
		union {
			DEFINE_MEMBER_N(float, ExtraFOV, 0x78);
		};

		static CVideoSettings* Get();
	};

	class CBaseCamera {
	public:
		union {
			DEFINE_MEMBER_N(GamePH::FreeCamera*, pFreeCamera, 0x20);
		};

		static CBaseCamera* Get();
	};

	class CLevel {
	public:
		union {
			DEFINE_MEMBER_N(GamePH::LevelDI*, pLevelDI, 0x20);
			DEFINE_MEMBER_N(CBaseCamera*, pCBaseCamera, 0x28);
		};

		static CLevel* Get();
	};

	class CGame {
	public:
		union {
			DEFINE_MEMBER_N(GamePH::GameDI_PH*, pGameDI_PH, 0x8);
			DEFINE_MEMBER_N(CVideoSettings*, pCVideoSettings, 0x28);
			DEFINE_MEMBER_N(CLevel*, pCLevel, 0x370);
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

	class CBulletPhysicsCharacter {
	public:
		float* MoveCharacter(Vector3* pos);

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