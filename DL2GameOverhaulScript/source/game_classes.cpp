#include <iostream>
#include <string_view>
#include "MinHook\include\MinHook.h"
#include "sigscan\offsets.h"
#include "menu\camera.h"
#include "game_classes.h"
#include "memory.h"

namespace Core {
	extern void OnPostUpdate();
}

namespace GamePH {
	#pragma region Misc
	void(*oOnPostUpdate)(LPVOID pGameDI_PH2) = nullptr;
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

			Hook::VTHook(pGameDI_PH2, &detourOnPostUpdate, reinterpret_cast<LPVOID*>(&oOnPostUpdate), 0x368);
			if (oOnPostUpdate)
				break;
		}
	}

	DWORD64(*pCalculateFreeCamCollision)(LPVOID pFreeCamera, float* finalPos) = nullptr;
	DWORD64(*oCalculateFreeCamCollision)(LPVOID pFreeCamera, float* finalPos) = nullptr;
	DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos) {
		if (!Menu::Camera::freeCamEnabled && !Menu::Camera::disablePhotoModeLimitsEnabled.value)
			return oCalculateFreeCamCollision(pFreeCamera, finalPos);

		return NULL;
	}
	void LoopHookCalculateFreeCamCollision() {
		while (true) {
			Sleep(250);

			if (!GamePH::pCalculateFreeCamCollision)
				GamePH::pCalculateFreeCamCollision = (decltype(GamePH::pCalculateFreeCamCollision))Offsets::Get_CalculateFreeCamCollisionOffset();
			else if (!GamePH::oCalculateFreeCamCollision && MH_CreateHook(GamePH::pCalculateFreeCamCollision, &GamePH::detourCalculateFreeCamCollision, reinterpret_cast<LPVOID*>(&GamePH::oCalculateFreeCamCollision)) == MH_OK) {
				MH_EnableHook(GamePH::pCalculateFreeCamCollision);
				break;
			}
		}
	}
	#pragma endregion

	#pragma region PlayerVariables
	DWORD64 PlayerVariables::FloatPlayerVariableVT;
	DWORD64 PlayerVariables::BoolPlayerVariableVT;

	std::vector <std::pair<std::string_view, std::pair<DWORD64, std::string_view>>> PlayerVariables::unorderedPlayerVars;
	bool PlayerVariables::gotPlayerVars = false;

	std::unique_ptr<Hook::BreakpointHook> PlayerVariables::loadPlayerFloatVarBpHook = nullptr;
	std::unique_ptr<Hook::BreakpointHook> PlayerVariables::loadPlayerBoolVarBpHook = nullptr;

	bool PlayerVariables::hooked = false;
	void PlayerVariables::RunHooks() {
		if (hooked)
			return;
		if (Offsets::Get_LoadPlayerFloatVariableOffset() == NULL)
			return;
		if (Offsets::Get_LoadPlayerBoolVariableOffset() == NULL)
			return;

		loadPlayerFloatVarBpHook = std::make_unique<Hook::BreakpointHook>(Offsets::Get_LoadPlayerFloatVariableOffset(), [&](PEXCEPTION_POINTERS info) -> void {
			const char* tempName = reinterpret_cast<const char*>(info->ContextRecord->R8);
			const std::string_view name = tempName;

			PlayerVariables::unorderedPlayerVars.emplace_back(name, std::make_pair(0x0, "float"));
		});
		loadPlayerBoolVarBpHook = std::make_unique<Hook::BreakpointHook>(Offsets::Get_LoadPlayerBoolVariableOffset(), [&](PEXCEPTION_POINTERS info) -> void {
			const char* tempName = reinterpret_cast<const char*>(info->ContextRecord->R8);
			const std::string_view name = tempName;

			PlayerVariables::unorderedPlayerVars.emplace_back(name, std::make_pair(0x0, "bool"));
		});

		hooked = true;
	}

	const DWORD64 PlayerVariables::GetFloatPlayerVariableVT() {
		if (FloatPlayerVariableVT != NULL)
			return FloatPlayerVariableVT;
		if (Offsets::Get_InitializePlayerVariablesOffset() == NULL)
			return NULL;

		const DWORD64 pInitializePlayerVariables = Offsets::Get_InitializePlayerVariablesOffset();
		const DWORD64 offsetToInstr = pInitializePlayerVariables + Offsets::Get_initPlayerFloatVarsInstrOffset() + 0x3; // 0x3 is instruction size
		const DWORD floatPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return FloatPlayerVariableVT = offsetToInstr + sizeof(DWORD) + floatPlayerVariableVTOffset;
	}
	const DWORD64 PlayerVariables::GetBoolPlayerVariableVT() {
		if (BoolPlayerVariableVT != NULL)
			return BoolPlayerVariableVT;
		if (Offsets::Get_InitializePlayerVariablesOffset() == NULL)
			return NULL;

		const DWORD64 pInitializePlayerVariables = Offsets::Get_InitializePlayerVariablesOffset();
		const DWORD64 offsetToInstr = pInitializePlayerVariables + Offsets::Get_initPlayerBoolVarsInstrOffset() + 0x3; // 0x3 is instruction size
		const DWORD boolPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return BoolPlayerVariableVT = offsetToInstr + sizeof(DWORD) + boolPlayerVariableVTOffset;
	}
	void PlayerVariables::GetPlayerVars() {
		if (gotPlayerVars)
			return;
		if (!Get())
			return;
		if (unorderedPlayerVars.empty())
			return;
		if (!GetFloatPlayerVariableVT())
			return;
		if (!GetBoolPlayerVariableVT())
			return;

		DWORD64** playerVars = reinterpret_cast<DWORD64**>(Get());

		for (auto it = unorderedPlayerVars.begin(); it != unorderedPlayerVars.end(); ++it) {
			while (true) {
				DWORD64 variableAddr = NULL;

				if (reinterpret_cast<DWORD64>(*playerVars) == GetFloatPlayerVariableVT()) {
					variableAddr = reinterpret_cast<DWORD64>(playerVars + 1);
					it->second.first = variableAddr;

					playerVars += 3;
					break;
				} else if (reinterpret_cast<DWORD64>(*playerVars) == GetBoolPlayerVariableVT()) {
					variableAddr = reinterpret_cast<DWORD64>(playerVars + 1);
					it->second.first = variableAddr;

					playerVars += 2;
					break;
				} else
					playerVars += 1;
			}
		}

		gotPlayerVars = true;
		loadPlayerFloatVarBpHook->Disable();
		loadPlayerBoolVarBpHook->Disable();
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
			if (Offsets::Get_PlayerStateOffset() == NULL)
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

	#pragma region FreeCamera
	void FreeCamera::AllowCameraMovement(int mode) {
		Memory::CallVT<203>(this, mode);
	}

	FreeCamera* FreeCamera::Get() {
		__try {
			Engine::CBaseCamera* pCBaseCamera = Engine::CBaseCamera::Get();
			if (!pCBaseCamera)
				return nullptr;

			FreeCamera* ptr = pCBaseCamera->pFreeCamera;

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region LevelDI
	float LevelDI::GetTimePlayed() {
		return Memory::CallVT<334, float>(this);
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

			GameDI_PH2* ptr = reinterpret_cast<GameDI_PH2*>(reinterpret_cast<DWORD64>(pGameDI_PH) + Offsets::Get_gameDI_PH2Offset());

			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;
			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	#pragma endregion

	#pragma region GameDI_PH
	DWORD64* GameDI_PH::GetLocalPlayerEntity() {
		return Memory::CallVT<135, DWORD64*>(this);
	}
	INT64 GameDI_PH::GetCurrentGameVersion() {
		return Memory::CallVT<218, INT64>(this);
	}
	void GameDI_PH::TogglePhotoMode(bool doNothing, bool setAsOptionalCamera) {
		Memory::CallVT<249>(this, doNothing, setAsOptionalCamera);
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
			if (Offsets::Get_g_PlayerObjProperties() == NULL)
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

	#pragma region CBaseCamera
	CBaseCamera* CBaseCamera::Get() {
		__try {
			CLevel* pCLevel = CLevel::Get();
			if (!pCLevel)
				return nullptr;

			CBaseCamera* ptr = pCLevel->pCBaseCamera;

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
			if (Offsets::Get_CLobbySteamOffset() == NULL)
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

	#pragma region CBulletPhysicsCharacter
	float* CBulletPhysicsCharacter::MoveCharacter(Vector3* pos) {
		float*(*MoveCharacter)(LPVOID, float*) = (decltype(MoveCharacter))Offsets::Get_MoveCharacterOffset();
		if (!MoveCharacter)
			return nullptr;

		return MoveCharacter(this, reinterpret_cast<float*>(pos));
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