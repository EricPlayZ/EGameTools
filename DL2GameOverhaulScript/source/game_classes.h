#pragma once
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <any>
#include "hook.h"
#include "utils.h"

#define STR_MERGE_IMPL(a, b) a##b
#define STR_MERGE(a, b) STR_MERGE_IMPL(a, b)
#define MAKE_PAD(size) STR_MERGE(_pad, __COUNTER__)[size]
#define DEFINE_MEMBER_N(type, name, offset) struct {unsigned char MAKE_PAD(offset); type name;}
#define DEFINE_MEMBER_0(type, name) struct {type name;}

// Game structs
struct Vector3 {
	float X, Y, Z;

	bool operator==(const Vector3& v) const {
		return Utils::are_same(X, v.X) && Utils::are_same(Y, v.Y) && Utils::are_same(Z, v.Z);
	}
	Vector3& operator+=(const Vector3& v) {
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		return *this;
	}
	Vector3& operator-=(const Vector3& v) {
		X -= v.X;
		Y -= v.Y;
		Z -= v.Z;
		return *this;
	}
	Vector3 operator+(const Vector3& v) const {
		return { X + v.X, Y + v.Y, Z + v.Z };
	}
	Vector3 operator-(const Vector3& v) const {
		return { X - v.X, Y - v.Y, Z - v.Z };
	}
	Vector3 operator*(float scalar) const {
		return { X * scalar, Y * scalar, Z * scalar };
	}
	Vector3 operator/(float scalar) const {
		return { X / scalar, Y / scalar, Z / scalar };
	}

	Vector3 normalize() {
		float length = std::sqrt(X * X + Y * Y + Z * Z);
		return { X / length, Y / length, Z / length };
	}
	Vector3 cross(const Vector3& v) const {
		return {
			Y * v.Z - Z * v.Y,
			Z * v.X - X * v.Z,
			X * v.Y - Y * v.X
		};
	}

	bool isDefault() const {
		return Utils::are_same(X, 0.0f) && Utils::are_same(Y, 0.0f) && Utils::are_same(Z, 0.0f);
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
	class CBaseCamera;
	class CoPhysicsProperty;
}

namespace GamePH {
	extern void LoopHookCreatePlayerHealthModule();
	extern void LoopHookOnUpdate();
	extern void LoopHookCalculateFreeCamCollision();
	extern void LoopHookLifeSetHealth();
	extern void LoopHookTogglePhotoMode();
	extern void LoopHookMoveCameraFromForwardUpPos();

	extern void ShowTPPModel(bool showTPPModel);

	class PlayerVariables {
	public:
		static std::vector<std::pair<std::string, std::pair<LPVOID, std::string>>> playerVars;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerVarsDefault;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerCustomVarsDefault;
		static bool gotPlayerVars;

		static void GetPlayerVars();
		static void SortPlayerVars();

		template <typename T>
		static void ChangePlayerVar(const std::string& playerVar, const T value) {
			static_assert(std::is_same<T, bool>::value || std::is_same<T, float>::value || std::is_same<T, std::string>::value, "Invalid type: value must be bool, float or string");

			auto it = std::find_if(PlayerVariables::playerVars.begin(), PlayerVariables::playerVars.end(), [&playerVar](const auto& pair) {
				return pair.first == playerVar;
			});

			if (it == PlayerVariables::playerVars.end())
				return;

			if (std::is_same<T, std::string>::value) {
				std::string valueStr = Utils::to_string(valueStr);
				if (it->second.second == "float") {
					float* const varValue = reinterpret_cast<float*>(it->second.first);
					const float actualValue = std::stof(valueStr);

					*varValue = actualValue;
					*(varValue + 1) = actualValue;
				}
				else {
					bool* const varValue = reinterpret_cast<bool*>(it->second.first);
					const bool actualValue = valueStr == "true";

					*varValue = actualValue;
					*(varValue + 1) = actualValue;
				}
			}
			else {
				T* const varValue = reinterpret_cast<T*>(it->second.first);

				*varValue = value;
				*(varValue + 1) = value;
			}
		}

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

	class TPPCameraDI {
	public:
		static TPPCameraDI* Get();
	};

	class CameraFPPDI {
	public:
		union {
			DEFINE_MEMBER_N(Engine::CBaseCamera*, pCBaseCamera, 0x38);
		};

		Vector3* GetForwardVector(Vector3* outForwardVec);
		Vector3* GetUpVector(Vector3* outUpVec);
		Vector3* GetPosition(Vector3* posIN);

		//static CameraFPPDI* Get();
	};

	class CoBaseCameraProxy {
	public:
		union {
			DEFINE_MEMBER_N(TPPCameraDI*, pTPPCameraDI, 0xD0);
		};
	};

	class FreeCamera {
	public:
		union {
			DEFINE_MEMBER_N(CoBaseCameraProxy*, pCoBaseCameraProxy, 0x18);
			DEFINE_MEMBER_N(Engine::CBaseCamera*, pCBaseCamera, 0x38);
			DEFINE_MEMBER_N(bool, enableSpeedMultiplier1, 0x42);
			DEFINE_MEMBER_N(bool, enableSpeedMultiplier2, 0x43);
			DEFINE_MEMBER_N(float, speedMultiplier, 0x1CC);
		};

		Vector3* GetForwardVector(Vector3* outForwardVec);
		Vector3* GetUpVector(Vector3* outUpVec);
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
		bool IsLoading();
		LPVOID GetViewCamera();
		void SetViewCamera(LPVOID viewCam);
		float GetTimePlayed();
		TimeWeather::CSystem* GetTimeWeatherSystem();

		static LevelDI* Get();
	};

	class gen_TPPModel {
	public:
		union {
			DEFINE_MEMBER_N(bool, enableTPPModel1, 0x2CF9);
			DEFINE_MEMBER_N(bool, enableTPPModel2, 0x2CFA);
		};

		static gen_TPPModel* Get();
	};

	class LocalClientDI {
	public:
		union {
			DEFINE_MEMBER_N(gen_TPPModel*, pgen_TPPModel, 0x90);
		};

		static LocalClientDI* Get();
	};

	class SessionCooperativeDI {
	public:
		union {
			DEFINE_MEMBER_N(LocalClientDI*, pLocalClientDI, 0xE08);
		};

		static SessionCooperativeDI* Get();
	};

	class GameDI_PH2 {
	public:
		static GameDI_PH2* Get();
	};

	class GameDI_PH {
	public:
		union {
			DEFINE_MEMBER_N(SessionCooperativeDI*, pSessionCooperativeDI, 0xF0);
			DEFINE_MEMBER_N(bool, blockPauseGameOnPlayerAfk, 0x830);
		};

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
			DEFINE_MEMBER_N(float, extraFOV, 0x7C);
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

	class CBaseCamera {
	public:
		union {
			DEFINE_MEMBER_N(float, yaw, 0x48);
			DEFINE_MEMBER_N(float, X, 0x4C);
			DEFINE_MEMBER_N(float, pitch, 0x58);
			DEFINE_MEMBER_N(float, Y, 0x5C);
			DEFINE_MEMBER_N(float, Z, 0x6C);
		};
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