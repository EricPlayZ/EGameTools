#include <iostream>
#include <string>
#include <thread>
#include "MinHook\include\MinHook.h"
#include "sigscan\offsets.h"
#include "menu\player.h"
#include "menu\camera.h"
#include "config\config.h"
#include "game_classes.h"
#include "memory.h"
#include "print.h"
#include "utils.h"

namespace Core {
	extern void OnPostUpdate();
}

#pragma region GamePH
namespace GamePH {
	#pragma region Hooks
	#pragma region CreatePlayerHealthModule
	static DWORD64(*pCreatePlayerHealthModule)(DWORD64 playerHealthModule) = nullptr;
	static DWORD64(*oCreatePlayerHealthModule)(DWORD64 playerHealthModule) = nullptr;
	DWORD64 detourCreatePlayerHealthModule(DWORD64 playerHealthModule) {
		PlayerHealthModule::pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(playerHealthModule);
		return oCreatePlayerHealthModule(playerHealthModule);
	}
	void LoopHookCreatePlayerHealthModule() {
		while (true) {
			Sleep(250);

			if (!pCreatePlayerHealthModule)
				pCreatePlayerHealthModule = (decltype(pCreatePlayerHealthModule))Offsets::Get_CreatePlayerHealthModuleOffset();
			else if (!oCreatePlayerHealthModule && MH_CreateHook(pCreatePlayerHealthModule, &detourCreatePlayerHealthModule, reinterpret_cast<LPVOID*>(&oCreatePlayerHealthModule)) == MH_OK) {
				MH_EnableHook(pCreatePlayerHealthModule);
				break;
			}
		}
	}
	#pragma endregion

	#pragma region OnPostUpdate
	static void(*oOnPostUpdate)(LPVOID pGameDI_PH2) = nullptr;
	void detourOnPostUpdate(LPVOID pGameDI_PH2) {
		oOnPostUpdate(pGameDI_PH2);
		Core::OnPostUpdate();
	}
	void LoopHookOnUpdate() {
		GamePH::GameDI_PH2* pGameDI_PH2 = nullptr;

		while (true) {
			Sleep(250);

			pGameDI_PH2 = GamePH::GameDI_PH2::Get();
			if (!pGameDI_PH2)
				continue;

			Hook::VTHook(pGameDI_PH2, &detourOnPostUpdate, reinterpret_cast<LPVOID*>(&oOnPostUpdate), 0x3a8);
			if (oOnPostUpdate)
				break;
		}
	}
	#pragma endregion

	#pragma region CalculateFreeCamCollision
	static DWORD64(*pCalculateFreeCamCollision)(LPVOID pFreeCamera, float* finalPos) = nullptr;
	static DWORD64(*oCalculateFreeCamCollision)(LPVOID pFreeCamera, float* finalPos) = nullptr;
	DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos) {
		if (!Menu::Camera::freeCamEnabled.value && !Menu::Camera::disablePhotoModeLimitsEnabled.value)
			return oCalculateFreeCamCollision(pFreeCamera, finalPos);

		return NULL;
	}
	void LoopHookCalculateFreeCamCollision() {
		while (true) {
			Sleep(250);

			if (!pCalculateFreeCamCollision)
				pCalculateFreeCamCollision = (decltype(pCalculateFreeCamCollision))Offsets::Get_CalculateFreeCamCollisionOffset();
			else if (!oCalculateFreeCamCollision && MH_CreateHook(pCalculateFreeCamCollision, &detourCalculateFreeCamCollision, reinterpret_cast<LPVOID*>(&oCalculateFreeCamCollision)) == MH_OK) {
				MH_EnableHook(pCalculateFreeCamCollision);
				break;
			}
		}
	}
	#pragma endregion

	#pragma region LifeSetHealth
	static void(*pLifeSetHealth)(float* pLifeHealth, float health) = nullptr;
	static void(*oLifeSetHealth)(float* pLifeHealth, float health) = nullptr;
	void detourLifeSetHealth(float* pLifeHealth, float health) {
		if (Menu::Player::godModeEnabled.value) {
			GamePH::PlayerHealthModule* playerHealthModule = GamePH::PlayerHealthModule::Get();
			if (playerHealthModule && (pLifeHealth + 1) == &playerHealthModule->health && playerHealthModule->health != 0)
				return;
		}
		oLifeSetHealth(pLifeHealth, health);
	}
	void LoopHookLifeSetHealth() {
		while (true) {
			Sleep(250);

			if (!pLifeSetHealth)
				pLifeSetHealth = (decltype(pLifeSetHealth))Offsets::Get_LifeSetHealth();
			else if (!oLifeSetHealth && MH_CreateHook(pLifeSetHealth, &detourLifeSetHealth, reinterpret_cast<LPVOID*>(&oLifeSetHealth)) == MH_OK) {
				MH_EnableHook(pLifeSetHealth);
				break;
			}
		}
	}
	#pragma endregion

	#pragma region TogglePhotoMode
	static void(*pTogglePhotoMode)(LPVOID guiPhotoModeData, bool enabled) = nullptr;
	static void(*oTogglePhotoMode)(LPVOID guiPhotoModeData, bool enabled) = nullptr;
	void detourTogglePhotoMode(LPVOID guiPhotoModeData, bool enabled) {
		Menu::Camera::photoModeEnabled = enabled;
		if (Menu::Camera::freeCamEnabled.value) {
			GamePH::GameDI_PH* pGameDI_PH = GamePH::GameDI_PH::Get();
			if (pGameDI_PH) {
				GamePH::FreeCamera* pFreeCam = GamePH::FreeCamera::Get();
				if (pFreeCam) {
					pGameDI_PH->TogglePhotoMode();
					pFreeCam->AllowCameraMovement(0);
				}
			}
		}

		oTogglePhotoMode(guiPhotoModeData, enabled);
	}
	void LoopHookTogglePhotoMode() {
		while (true) {
			Sleep(250);

			if (!pTogglePhotoMode)
				pTogglePhotoMode = (decltype(pTogglePhotoMode))Offsets::Get_TogglePhotoMode();
			else if (!oTogglePhotoMode && MH_CreateHook(pTogglePhotoMode, &detourTogglePhotoMode, reinterpret_cast<LPVOID*>(&oTogglePhotoMode)) == MH_OK) {
				MH_EnableHook(pTogglePhotoMode);
				break;
			}
		}
	}
	#pragma endregion

	#pragma region MoveCamera
	static void(*pMoveCamera)(LPVOID pCBaseCamera, Vector3* pos, float* a3, float* a4) = nullptr;
	static void(*oMoveCamera)(LPVOID pCBaseCamera, Vector3* pos, float* a3, float* a4) = nullptr;
	void detourMoveCamera(LPVOID pCBaseCamera, Vector3* pos, float* a3, float* a4) {
		if (Menu::Camera::thirdPersonCameraEnabled && !Menu::Camera::freeCamEnabled.value) {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel) {
				oMoveCamera(pCBaseCamera, pos, a3, a4);
				return;
			}
			CameraFPPDI* viewCam = static_cast<CameraFPPDI*>(iLevel->GetViewCamera());
			if (!viewCam) {
				oMoveCamera(pCBaseCamera, pos, a3, a4);
				return;
			}
			Engine::CBulletPhysicsCharacter* playerCharacter = Engine::CBulletPhysicsCharacter::Get();

			Vector3 forwardVec{};
			viewCam->GetForwardVector(&forwardVec);
			const Vector3 normForwardVec = forwardVec.normalize();

			Vector3 newCamPos = (!playerCharacter ? *pos : playerCharacter->playerPos) - normForwardVec * -Menu::Camera::DistanceBehindPlayer;
			newCamPos.Y += Menu::Camera::HeightAbovePlayer - (!playerCharacter ? 1.0f : 0.0f);

			*pos = newCamPos;
		}

		oMoveCamera(pCBaseCamera, pos, a3, a4);
	}
	void LoopHookMoveCamera() {
		while (true) {
			Sleep(250);

			if (!pMoveCamera)
				pMoveCamera = (decltype(pMoveCamera))Offsets::Get_MoveCamera();
			else if (!oMoveCamera && MH_CreateHook(pMoveCamera, &detourMoveCamera, reinterpret_cast<LPVOID*>(&oMoveCamera)) == MH_OK) {
				MH_EnableHook(pMoveCamera);
				break;
			}
		}
	}
	#pragma endregion
	#pragma endregion

	#pragma region PlayerVariables
	std::vector <std::pair<std::string, std::pair<LPVOID, std::string>>> PlayerVariables::playerVars;
	std::vector <std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerVarsDefault;
	std::vector <std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerCustomVarsDefault;
	bool PlayerVariables::gotPlayerVars = false;

	void PlayerVariables::SortPlayerVars() {
		if (!playerVars.empty())
			return;

		std::stringstream ss(Config::playerVars);

		while (ss.good()) {
			std::string pVar{};
			getline(ss, pVar, ',');

			std::stringstream ssPVar(pVar);

			std::string varName{};
			std::string varType{};

			while (ssPVar.good()) {
				std::string subStr{};
				getline(ssPVar, subStr, ':');

				if (subStr != "float" && subStr != "bool")
					varName = subStr;
				else
					varType = subStr;
			}

			PlayerVariables::playerVars.emplace_back(varName, std::make_pair(nullptr, varType));
			PlayerVariables::playerVarsDefault.emplace_back(varName, std::make_pair(varType == "float" ? 0.0f : false, varType));
			PlayerVariables::playerCustomVarsDefault.emplace_back(varName, std::make_pair(varType == "float" ? 0.0f : false, varType));
		}
	}

	PDWORD64 PlayerVariables::GetFloatPlayerVariableVT() {
		if (!Offsets::Get_InitializePlayerVariablesOffset())
			return nullptr;

		const DWORD64 offsetToInstr = Offsets::Get_InitializePlayerVariablesOffset() + Offsets::Get_initPlayerFloatVarsInstrOffset() + 0x3; // 0x3 is instruction size
		const DWORD floatPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return reinterpret_cast<PDWORD64>(offsetToInstr + sizeof(DWORD) + floatPlayerVariableVTOffset);
	}
	PDWORD64 PlayerVariables::GetBoolPlayerVariableVT() {
		if (!Offsets::Get_InitializePlayerVariablesOffset())
			return nullptr;

		const DWORD64 offsetToInstr = Offsets::Get_InitializePlayerVariablesOffset() + Offsets::Get_initPlayerBoolVarsInstrOffset() + 0x3; // 0x3 is instruction size
		const DWORD boolPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return reinterpret_cast<PDWORD64>(offsetToInstr + sizeof(DWORD) + boolPlayerVariableVTOffset);
	}
	void PlayerVariables::GetPlayerVars() {
		if (gotPlayerVars)
			return;
		if (!Get())
			return;
		if (playerVars.empty())
			return;
		if (!GetFloatPlayerVariableVT())
			return;
		if (!GetBoolPlayerVariableVT())
			return;

		PDWORD64* playerVarsMem = reinterpret_cast<PDWORD64*>(Get());
		bool isFloatPlayerVar = false;
		bool isBoolPlayerVar = false;

		for (auto it = playerVars.begin(); it != playerVars.end(); ++it) {
			while (true) {
				isFloatPlayerVar = *playerVarsMem == GetFloatPlayerVariableVT();
				isBoolPlayerVar = *playerVarsMem == GetBoolPlayerVariableVT();

				if (isFloatPlayerVar || isBoolPlayerVar) {
					it->second.first = playerVarsMem + 1;
					const std::string varName = it->first;

					auto itDef = std::find_if(GamePH::PlayerVariables::playerVarsDefault.begin(), GamePH::PlayerVariables::playerVarsDefault.end(), [&varName](const auto& pair) {
						return pair.first == varName;
					});
					if (itDef != GamePH::PlayerVariables::playerVarsDefault.end()) {
						if (isFloatPlayerVar)
							itDef->second.first = *reinterpret_cast<float*>(it->second.first);
						else
							itDef->second.first = *reinterpret_cast<bool*>(it->second.first);
					}

					auto itCustomDef = std::find_if(GamePH::PlayerVariables::playerCustomVarsDefault.begin(), GamePH::PlayerVariables::playerCustomVarsDefault.end(), [&varName](const auto& pair) {
						return pair.first == varName;
					});
					if (itCustomDef != GamePH::PlayerVariables::playerCustomVarsDefault.end()) {
						if (isFloatPlayerVar)
							itCustomDef->second.first = *reinterpret_cast<float*>(it->second.first);
						else
							itCustomDef->second.first = *reinterpret_cast<bool*>(it->second.first);
					}

					playerVarsMem += isFloatPlayerVar ? 3 : 2;
					break;
				} else
					playerVarsMem += 1;
			}
		}

		gotPlayerVars = true;
	}

	PlayerVariables* PlayerVariables::Get() {
		__try {
			PlayerState* pPlayerState = PlayerState::Get();
			if (!pPlayerState)
				return nullptr;

			PlayerVariables* ptr = pPlayerState->playerVars;

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region PlayerState
	PlayerState* PlayerState::Get() {
		__try {
			if (!Offsets::Get_PlayerStateOffset())
				return nullptr;

			PlayerState* ptr = *reinterpret_cast<PlayerState**>(Offsets::Get_PlayerStateOffset());

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region PlayerHealthModule
	PlayerHealthModule* PlayerHealthModule::pPlayerHealthModule = nullptr;
	PlayerHealthModule* PlayerHealthModule::Get() {
		__try {
			if (!pPlayerHealthModule)
				return nullptr;
			if (!*reinterpret_cast<PDWORD64*>(pPlayerHealthModule))
				return nullptr;

			return pPlayerHealthModule;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			pPlayerHealthModule = nullptr;
			return nullptr;
		}
	}
	#pragma endregion

	Vector3* (*pGetForwardVector)(LPVOID viewCam, Vector3* outForwardVec) = nullptr;
	Vector3* (*pGetUpVector)(LPVOID viewCam, Vector3* outUpVec) = nullptr;
	#pragma region CameraFPPDI
	Vector3* CameraFPPDI::GetForwardVector(Vector3* outForwardVec) {
		if (!pGetForwardVector) {
			pGetForwardVector = (decltype(pGetForwardVector))Offsets::Get_GetForwardVector();
			return nullptr;
		}
		return pGetForwardVector(this, outForwardVec);
	}
	Vector3* CameraFPPDI::GetUpVector(Vector3* outUpVec) {
		if (!pGetUpVector) {
			pGetUpVector = (decltype(pGetUpVector))Offsets::Get_GetUpVector();
			return nullptr;
		}
		return pGetUpVector(this, outUpVec);
	}
	Vector3* CameraFPPDI::GetPosition(Vector3* posIN) {
		return Memory::CallVT<181, Vector3*>(this, posIN);
	}

	CameraFPPDI* CameraFPPDI::Get() {
		__try {
			PDWORD64 pg_CameraFPPDI = Offsets::Get_g_CameraFPPDI();
			if (!pg_CameraFPPDI)
				return nullptr;

			CameraFPPDI* ptr = reinterpret_cast<CameraFPPDI*>(*pg_CameraFPPDI);

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region FreeCamera
	Vector3* FreeCamera::GetForwardVector(Vector3* outForwardVec) {
		if (!pGetForwardVector) {
			pGetForwardVector = (decltype(pGetForwardVector))Offsets::Get_GetForwardVector();
			return nullptr;
		}
		return pGetForwardVector(this, outForwardVec);
	}
	Vector3* FreeCamera::GetUpVector(Vector3* outUpVec) {
		if (!pGetUpVector) {
			pGetUpVector = (decltype(pGetUpVector))Offsets::Get_GetUpVector();
			return nullptr;
		}
		return pGetUpVector(this, outUpVec);
	}
	Vector3* FreeCamera::GetPosition(Vector3* posIN) {
		return Memory::CallVT<181, Vector3*>(this, posIN);
	}
	void FreeCamera::AllowCameraMovement(int mode) {
		Memory::CallVT<187>(this, mode);
	}

	FreeCamera* FreeCamera::Get() {
		__try {
			PDWORD64 pg_FreeCamera = Offsets::Get_g_FreeCamera();
			if (!pg_FreeCamera)
				return nullptr;

			FreeCamera* ptr = reinterpret_cast<FreeCamera*>(*pg_FreeCamera);

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region DayNightCycle
	void DayNightCycle::SetDaytime(float time) {
		time /= 24;
		time1 = time;
		time2 = time;
		time3 = time;
	}

	DayNightCycle* DayNightCycle::Get() {
		__try {
			if (!Offsets::Get_g_DayNightCycle())
				return nullptr;

			DayNightCycle* ptr = *reinterpret_cast<DayNightCycle**>(Offsets::Get_g_DayNightCycle());

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region TimeWeather
	namespace TimeWeather {
		#pragma region CSystem
		void CSystem::SetForcedWeather(int weather) {
			if (!Offsets::Get_SetForcedWeatherOffset())
				return;

			void(*pSetForcedWeather)(LPVOID timeWeatherSystem, int weather) = (decltype(pSetForcedWeather))Offsets::Get_SetForcedWeatherOffset();

			if (!pSetForcedWeather)
				return;
			pSetForcedWeather(this, weather);
		}
		int CSystem::GetCurrentWeather() {
			if (!Offsets::Get_GetCurrentWeatherOffset())
				return EWeather::TYPE::Default;

			int(*pGetCurrentWeather)(LPVOID timeWeatherSystem) = (decltype(pGetCurrentWeather))Offsets::Get_GetCurrentWeatherOffset();

			if (!pGetCurrentWeather)
				return EWeather::TYPE::Default;
			return pGetCurrentWeather(this);
		}

		CSystem* CSystem::Get() {
			__try {
				LevelDI* pLevelDI = LevelDI::Get();
				if (!pLevelDI)
					return nullptr;

				CSystem* ptr = pLevelDI->GetTimeWeatherSystem();

				if (!Memory::IsValidPtr(ptr))
					return nullptr;
				return ptr;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				return nullptr;
			}
		}
		#pragma endregion
	}
	#pragma endregion

	#pragma region LevelDI
	float LevelDI::GetTimePlayed() {
		return Memory::CallVT<317, float>(this);
	}
	LPVOID LevelDI::GetViewCamera() {
		if (!Offsets::Get_GetViewCameraOffset())
			return nullptr;

		LPVOID(*pGetViewCamera)(LPVOID iLevel) = (decltype(pGetViewCamera))Offsets::Get_GetViewCameraOffset();

		if (!pGetViewCamera)
			return nullptr;
		return pGetViewCamera(this);
	}
	TimeWeather::CSystem* LevelDI::GetTimeWeatherSystem() {
		__try {
			if (!Offsets::Get_GetTimeWeatherSystemOffset())
				return nullptr;

			TimeWeather::CSystem*(*pGetTimeWeatherSystem)(LevelDI* iLevel) = (decltype(pGetTimeWeatherSystem))Offsets::Get_GetTimeWeatherSystemOffset();

			if (!pGetTimeWeatherSystem)
				return nullptr;
			return pGetTimeWeatherSystem(this);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}

	LevelDI* LevelDI::Get() {
		__try {
			Engine::CLevel* pCLevel = Engine::CLevel::Get();
			if (!pCLevel)
				return nullptr;

			LevelDI* ptr = pCLevel->pLevelDI;

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region GameDI_PH2
	GameDI_PH2* GameDI_PH2::Get() {
		__try {
			GameDI_PH* pGameDI_PH = GameDI_PH::Get();
			if (!pGameDI_PH)
				return nullptr;

			GameDI_PH2* ptr = reinterpret_cast<GameDI_PH2*>(reinterpret_cast<DWORD64>(pGameDI_PH) + Offsets::Get_gameDI_PH2_offset());

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region GameDI_PH
	INT64 GameDI_PH::GetCurrentGameVersion() {
		return Memory::CallVT<225, INT64>(this);
	}
	void GameDI_PH::TogglePhotoMode(bool doNothing, bool setAsOptionalCamera) {
		Memory::CallVT<258>(this, doNothing, setAsOptionalCamera);
	}

	GameDI_PH* GameDI_PH::Get() {
		__try {
			Engine::CGame* pCGame = Engine::CGame::Get();
			if (!pCGame)
				return nullptr;

			GameDI_PH* ptr = pCGame->pGameDI_PH;

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region PlayerObjProperties
	PlayerObjProperties* PlayerObjProperties::Get() {
		__try {
			if (!Offsets::Get_g_PlayerObjProperties())
				return nullptr;

			PlayerObjProperties* ptr = *reinterpret_cast<PlayerObjProperties**>(Offsets::Get_g_PlayerObjProperties());

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion
}
#pragma endregion

#pragma region Engine
namespace Engine {
	#pragma region CVideoSettings
	CVideoSettings* CVideoSettings::Get() {
		__try {
			CGame* pCGame = CGame::Get();
			if (!pCGame)
				return nullptr;

			CVideoSettings* ptr = pCGame->pCVideoSettings;

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CLevel
	CLevel* CLevel::Get() {
		__try {
			CGame* pCGame = CGame::Get();
			if (!pCGame)
				return nullptr;

			CLevel* ptr = pCGame->pCLevel;

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CGame
	CGame* CGame::Get() {
		__try {
			CLobbySteam* pCLobbySteam = CLobbySteam::Get();
			if (!pCLobbySteam)
				return nullptr;

			CGame* ptr = pCLobbySteam->pCGame;

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CLobbySteam
	CLobbySteam* CLobbySteam::Get() {
		__try {
			if (!Offsets::Get_CLobbySteamOffset())
				return nullptr;

			CLobbySteam* ptr = *reinterpret_cast<CLobbySteam**>(Offsets::Get_CLobbySteamOffset());

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CInput
	DWORD64 CInput::BlockGameInput() {
		return Memory::CallVT<2, DWORD64>(this);
	}
	void CInput::UnlockGameInput() {
		Memory::CallVT<1>(this);
	}

	CInput* CInput::Get() {
		__try {
			if (!Offsets::Get_g_CInput())
				return nullptr;

			CInput* ptr = *reinterpret_cast<CInput**>(Offsets::Get_g_CInput());

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CBulletPhysicsCharacter
	Vector3 CBulletPhysicsCharacter::posBeforeFreeze{};

	void CBulletPhysicsCharacter::FreezeCharacter() {
		MoveCharacter(posBeforeFreeze);
	}
	void CBulletPhysicsCharacter::MoveCharacter(const Vector3& pos) {
		playerDownwardVelocity = 0.0f;
		playerPos = pos;
		playerPos2 = pos;
	}

	CBulletPhysicsCharacter* CBulletPhysicsCharacter::Get() {
		__try {
			CoPhysicsProperty* pCoPhysicsProperty = CoPhysicsProperty::Get();
			if (!pCoPhysicsProperty)
				return nullptr;

			CBulletPhysicsCharacter* ptr = pCoPhysicsProperty->pCBulletPhysicsCharacter;

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region CoPhysicsProperty
	CoPhysicsProperty* CoPhysicsProperty::Get() {
		__try {
			GamePH::PlayerObjProperties* pPlayerObjProperties = GamePH::PlayerObjProperties::Get();
			if (!pPlayerObjProperties)
				return nullptr;

			CoPhysicsProperty* ptr = pPlayerObjProperties->pCoPhysicsProperty;

			if (!Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion
}
#pragma endregion