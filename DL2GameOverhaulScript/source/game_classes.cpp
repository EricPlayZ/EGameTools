#include <iostream>
#include <string>
#include <thread>
#include "config\config.h"
#include "game_classes.h"
#include "memory.h"
#include "menu\camera.h"
#include "menu\misc.h"
#include "menu\player.h"
#include "print.h"
#include "sigscan\offsets.h"
#include "utils.h"

namespace Core {
	extern void OnPostUpdate();
}

#pragma region GamePH
namespace GamePH {
#pragma region Hooks
	// Forward decl
	static DWORD64 detourCreatePlayerHealthModule(DWORD64 playerHealthModule);
	static void detourOnPostUpdate(LPVOID pGameDI_PH2);
	static DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos);
	static void detourLifeSetHealth(float* pLifeHealth, float health);
	static void detourTogglePhotoMode(LPVOID guiPhotoModeData, bool enabled);
	static void detourShowTPPModelFunc3(DWORD64 a1, bool showTPPModel);
	static void detourMoveCameraFromForwardUpPos(LPVOID pCBaseCamera, float* a3, float* a4, Vector3* pos);
	static bool detourIsNotOutOfBounds(LPVOID pInstance, DWORD64 a2);
	static void detourShowUIManager(LPVOID pLevelDI, bool enabled);

#pragma region CreatePlayerHealthModule
	static Hook::MHook<LPVOID, DWORD64(*)(DWORD64)> CreatePlayerHealthModuleHook{ &Offsets::Get_CreatePlayerHealthModule, &detourCreatePlayerHealthModule };

	static DWORD64 detourCreatePlayerHealthModule(DWORD64 playerHealthModule) {
		PlayerHealthModule::pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(playerHealthModule);
		return CreatePlayerHealthModuleHook.pOriginal(playerHealthModule);
	}
#pragma endregion

#pragma region OnPostUpdate
	static Hook::VTHook<GamePH::GameDI_PH2*, void(*)(LPVOID)> OnPostUpdateHook{ &GamePH::GameDI_PH2::Get, &detourOnPostUpdate, 0x3A8 };

	static void detourOnPostUpdate(LPVOID pGameDI_PH2) {
		OnPostUpdateHook.pOriginal(pGameDI_PH2);
		Core::OnPostUpdate();
	}
#pragma endregion

#pragma region CalculateFreeCamCollision
	static Hook::MHook<LPVOID, DWORD64(*)(LPVOID, float*)> CalculateFreeCamCollisionHook{ &Offsets::Get_CalculateFreeCamCollision, &detourCalculateFreeCamCollision };

	static DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos) {
		if (Menu::Camera::disablePhotoModeLimits.GetValue() || Menu::Camera::freeCam.GetValue())
			return 0;

		return CalculateFreeCamCollisionHook.pOriginal(pFreeCamera, finalPos);
	}
#pragma endregion

#pragma region LifeSetHealth
	static Hook::MHook<LPVOID, void(*)(float*, float)> LifeSetHealthHook{ &Offsets::Get_LifeSetHealth, &detourLifeSetHealth };

	static void detourLifeSetHealth(float* pLifeHealth, float health) {
		if (!Menu::Player::godMode.GetValue() && !Menu::Camera::freeCam.GetValue())
			return LifeSetHealthHook.pOriginal(pLifeHealth, health);

		GamePH::PlayerHealthModule* playerHealthModule = GamePH::PlayerHealthModule::Get();
		if (!playerHealthModule)
			return LifeSetHealthHook.pOriginal(pLifeHealth, health);
		GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return LifeSetHealthHook.pOriginal(pLifeHealth, health);
				
		if (std::abs(reinterpret_cast<LONG64>(playerHealthModule) - reinterpret_cast<LONG64>(pLifeHealth)) < 0x100 && playerHealthModule->health > 0.0f)
			return;

		LifeSetHealthHook.pOriginal(pLifeHealth, health);
	}
#pragma endregion

#pragma region TogglePhotoMode
	static Hook::MHook<LPVOID, void(*)(LPVOID, bool)> TogglePhotoModeHook{ &Offsets::Get_TogglePhotoMode, &detourTogglePhotoMode };

	static void detourTogglePhotoMode(LPVOID guiPhotoModeData, bool enabled) {
		Menu::Camera::photoMode.Set(enabled);

		if (!Menu::Camera::freeCam.GetValue())
			return TogglePhotoModeHook.pOriginal(guiPhotoModeData, enabled);
		GamePH::GameDI_PH* pGameDI_PH = GamePH::GameDI_PH::Get();
		if (!pGameDI_PH)
			return TogglePhotoModeHook.pOriginal(guiPhotoModeData, enabled);
		GamePH::FreeCamera* pFreeCam = GamePH::FreeCamera::Get();
		if (!pFreeCam)
			return TogglePhotoModeHook.pOriginal(guiPhotoModeData, enabled);

		if (enabled) {
			pGameDI_PH->TogglePhotoMode();
			pFreeCam->AllowCameraMovement(0);
		}
		
		TogglePhotoModeHook.pOriginal(guiPhotoModeData, enabled);
	}
#pragma endregion

#pragma region ShowTPPModelFunc3
	static Option wannaUseTPPModel{};
	static Hook::MHook<LPVOID, void(*)(DWORD64, bool)> ShowTPPModelFunc3Hook{ &Offsets::Get_ShowTPPModelFunc3, &detourShowTPPModelFunc3 };

	static void detourShowTPPModelFunc3(DWORD64 a1, bool showTPPModel) {
		wannaUseTPPModel.Set(showTPPModel);

		gen_TPPModel* pgen_TPPModel = gen_TPPModel::Get();
		if (!pgen_TPPModel) {
			ShowTPPModelFunc3Hook.pOriginal(a1, showTPPModel);
			return;
		}
		if (wannaUseTPPModel.HasChangedTo(false)) {
			wannaUseTPPModel.SetPrevValue(false);
			pgen_TPPModel->enableTPPModel2 = true;
			pgen_TPPModel->enableTPPModel1 = true;
		}
		ShowTPPModelFunc3Hook.pOriginal(a1, showTPPModel);
	}
#pragma endregion

#pragma region MoveCameraFromForwardUpPos
	static Hook::MHook<LPVOID, void(*)(LPVOID, float*, float*, Vector3*)> MoveCameraFromForwardUpPosHook{ &Offsets::Get_MoveCameraFromForwardUpPos, &detourMoveCameraFromForwardUpPos };

	static void detourMoveCameraFromForwardUpPos(LPVOID pCBaseCamera, float* a3, float* a4, Vector3* pos) {
		GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
		if (!iLevel || !iLevel->IsLoaded())
			return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

		gen_TPPModel* pgen_TPPModel = gen_TPPModel::Get();
		if (pgen_TPPModel) {
			if (wannaUseTPPModel.GetValue()) {
				wannaUseTPPModel.SetPrevValue(true);
				if (Menu::Camera::thirdPersonCamera.GetValue() && Menu::Camera::tpUseTPPModel.GetValue()) {
					pgen_TPPModel->enableTPPModel2 = false;
					pgen_TPPModel->enableTPPModel1 = false;
				}
			}

			if (Menu::Camera::photoMode.HasChangedTo(false) && !wannaUseTPPModel.GetValue()) {
				if (!Menu::Camera::freeCam.GetValue() && !Menu::Camera::tpUseTPPModel.GetValue()) {
					Menu::Camera::photoMode.SetPrevValue(false);
					ShowTPPModel(false);
				} else if (Menu::Camera::freeCam.GetValue() || (Menu::Camera::tpUseTPPModel.GetValue() && Menu::Camera::thirdPersonCamera.GetValue())) {
					Menu::Camera::photoMode.SetPrevValue(true);
					ShowTPPModel(true);
				}
			} else if (Menu::Camera::photoMode.HasChangedTo(true)) {
				Menu::Camera::photoMode.SetPrevValue(true);
				ShowTPPModel(true);
			} else if (Menu::Camera::freeCam.HasChangedTo(false)) {
				if (!Menu::Camera::photoMode.GetValue() && !Menu::Camera::thirdPersonCamera.GetValue()) {
					Menu::Camera::freeCam.SetPrevValue(false);
					ShowTPPModel(false);
				} else if (Menu::Camera::photoMode.GetValue() || (Menu::Camera::tpUseTPPModel.GetValue() && Menu::Camera::thirdPersonCamera.GetValue())) {
					Menu::Camera::freeCam.SetPrevValue(true);
					ShowTPPModel(true);
				}
			} else if (Menu::Camera::freeCam.HasChangedTo(true)) {
				Menu::Camera::freeCam.SetPrevValue(true);
				ShowTPPModel(true);
			} else if (Menu::Camera::thirdPersonCamera.HasChangedTo(false)) {
				if (!Menu::Camera::freeCam.GetValue() && !Menu::Camera::photoMode.GetValue()) {
					Menu::Camera::thirdPersonCamera.SetPrevValue(false);
					ShowTPPModel(false);
				} else if (Menu::Camera::freeCam.GetValue() || Menu::Camera::photoMode.GetValue()) {
					Menu::Camera::thirdPersonCamera.SetPrevValue(true);
					ShowTPPModel(true);
				}
			} else if (Menu::Camera::thirdPersonCamera.HasChangedTo(true) && Menu::Camera::tpUseTPPModel.GetValue()) {
				Menu::Camera::thirdPersonCamera.SetPrevValue(true);
				ShowTPPModel(true);
			} else if (Menu::Camera::tpUseTPPModel.HasChangedTo(false)) {
				if (!Menu::Camera::freeCam.GetValue() && !Menu::Camera::photoMode.GetValue()) {
					Menu::Camera::tpUseTPPModel.SetPrevValue(false);
					ShowTPPModel(false);
				} else if (Menu::Camera::freeCam.GetValue() || Menu::Camera::photoMode.GetValue()) {
					Menu::Camera::tpUseTPPModel.SetPrevValue(true);
					ShowTPPModel(true);
				}
			} else if (Menu::Camera::tpUseTPPModel.HasChangedTo(true) && Menu::Camera::thirdPersonCamera.GetValue()) {
				Menu::Camera::tpUseTPPModel.SetPrevValue(true);
				ShowTPPModel(true);
			}
		}

		if (!Menu::Camera::thirdPersonCamera.GetValue() || Menu::Camera::photoMode.GetValue() || Menu::Camera::freeCam.GetValue() || !pos)
			return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

		CameraFPPDI* viewCam = static_cast<CameraFPPDI*>(iLevel->GetViewCamera());
		if (!viewCam)
			return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

		Vector3 forwardVec{};
		viewCam->GetForwardVector(&forwardVec);
		const Vector3 normForwardVec = forwardVec.normalize();

		Vector3 newCamPos = *pos - normForwardVec * -Menu::Camera::tpDistanceBehindPlayer;
		newCamPos.Y += Menu::Camera::tpHeightAbovePlayer - 1.5f;

		*pos = newCamPos;

		MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);
	}
#pragma endregion

#pragma region IsNotOutOfBounds
	static Hook::MHook<LPVOID, bool(*)(LPVOID, DWORD64)> IsNotOutOfBoundsHook{ &Offsets::Get_IsNotOutOfBounds, &detourIsNotOutOfBounds };

	static bool detourIsNotOutOfBounds(LPVOID pInstance, DWORD64 a2) {
		if (Menu::Player::disableOutOfBoundsTimer.GetValue())
			return true;

		return IsNotOutOfBoundsHook.pOriginal(pInstance, a2);
	}
#pragma endregion

#pragma region ShowUIManager
	static LPVOID GetShowUIManager() {
		return Utils::GetProcAddr("engine_x64_rwdi.dll", "?ShowUIManager@ILevel@@QEAAX_N@Z");
	}
	static Hook::MHook<LPVOID, void(*)(LPVOID, bool)> ShowUIManagerHook{ &GetShowUIManager, &detourShowUIManager };

	static void detourShowUIManager(LPVOID pLevelDI, bool enabled) {
		if (Menu::Misc::disableHUD.GetValue())
			enabled = false;

		ShowUIManagerHook.pOriginal(pLevelDI, enabled);
	}
#pragma endregion
#pragma endregion

#pragma region OtherFuncs
	static DWORD64 ShowTPPModelFunc2(LPVOID a1) {
		DWORD64(*pShowTPPModelFunc2)(LPVOID a1) = (decltype(pShowTPPModelFunc2))Offsets::Get_ShowTPPModelFunc2();
		if (!pShowTPPModelFunc2)
			return 0;

		return pShowTPPModelFunc2(a1);
	}
	void ShowTPPModel(bool showTPPModel) {
		GameDI_PH* pGameDI_PH = GameDI_PH::Get();
		if (!pGameDI_PH)
			return;
		DWORD64 tppFunc2Addr = ShowTPPModelFunc2(pGameDI_PH);
		if (!tppFunc2Addr)
			return;
		if (!ShowTPPModelFunc3Hook.pTarget)
			return;
		gen_TPPModel* pgen_TPPModel = gen_TPPModel::Get();
		if (!pgen_TPPModel)
			return;
		
		ShowTPPModelFunc3Hook.pTarget(tppFunc2Addr, showTPPModel);
	}
#pragma endregion

#pragma region PlayerVariables
	static const int FLOAT_VAR_OFFSET = 3;
	static const int BOOL_VAR_OFFSET = 2;
	static const int VAR_LOC_OFFSET = 1;

	std::vector<std::pair<std::string, std::pair<LPVOID, std::string>>> PlayerVariables::playerVars;
	std::vector<std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerVarsDefault;
	std::vector<std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerCustomVarsDefault;
	bool PlayerVariables::gotPlayerVars = false;

	static PDWORD64 getFloatPlayerVariableVT() {
		if (!Offsets::Get_InitializePlayerVariables())
			return nullptr;

		const DWORD64 offsetToInstr = Offsets::Get_InitializePlayerVariables() + Offsets::Get_initPlayerFloatVarsInstr_offset() + 0x3; // 0x3 is instruction size
		const DWORD floatPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return reinterpret_cast<PDWORD64>(offsetToInstr + sizeof(DWORD) + floatPlayerVariableVTOffset);
	}
	static PDWORD64 getBoolPlayerVariableVT() {
		if (!Offsets::Get_InitializePlayerVariables())
			return nullptr;

		const DWORD64 offsetToInstr = Offsets::Get_InitializePlayerVariables() + Offsets::Get_initPlayerBoolVarsInstr_offset() + 0x3; // 0x3 is instruction size
		const DWORD boolPlayerVariableVTOffset = *reinterpret_cast<DWORD*>(offsetToInstr);

		return reinterpret_cast<PDWORD64>(offsetToInstr + sizeof(DWORD) + boolPlayerVariableVTOffset);
	}

	template <typename T>
	static void updateDefaultVar(std::vector<std::pair<std::string, std::pair<std::any, std::string>>>& defaultVars, const std::string& varName, T varValue) {
		static_assert(std::is_same<T, float>::value || std::is_same<T, bool>::value, "Invalid type: value must be float or bool");

		auto it = std::find_if(defaultVars.begin(), defaultVars.end(), [&varName](const auto& pair) {
			return pair.first == varName;
		});
		if (it == defaultVars.end())
			return;

		it->second.first.template emplace<T>(varValue);
	}
	static void processPlayerVar(PDWORD64*& playerVarsMem, std::pair<std::string, std::pair<LPVOID, std::string>>& var) {
		while (true) {
			const bool isFloatPlayerVar = *playerVarsMem == getFloatPlayerVariableVT();
			const bool isBoolPlayerVar = *playerVarsMem == getBoolPlayerVariableVT();

			if (isFloatPlayerVar || isBoolPlayerVar) {
				var.second.first = playerVarsMem + VAR_LOC_OFFSET;
				const std::string& varName = var.first;

				if (isFloatPlayerVar) {
					float* varValue = reinterpret_cast<float*>(var.second.first);
					updateDefaultVar(GamePH::PlayerVariables::playerVarsDefault, varName, *varValue);
					updateDefaultVar(GamePH::PlayerVariables::playerCustomVarsDefault, varName, *varValue);

					playerVarsMem += FLOAT_VAR_OFFSET;
				}
				else {
					bool* varValue = reinterpret_cast<bool*>(var.second.first);
					updateDefaultVar(GamePH::PlayerVariables::playerVarsDefault, varName, *varValue);
					updateDefaultVar(GamePH::PlayerVariables::playerCustomVarsDefault, varName, *varValue);

					playerVarsMem += BOOL_VAR_OFFSET;
				}

				break;
			}
			else
				playerVarsMem += 1;
		}
	}

	void PlayerVariables::GetPlayerVars() {
		if (gotPlayerVars)
			return;
		if (!Get())
			return;
		if (playerVars.empty())
			return;
		if (!getFloatPlayerVariableVT())
			return;
		if (!getBoolPlayerVariableVT())
			return;

		PDWORD64* playerVarsMem = reinterpret_cast<PDWORD64*>(Get());

		for (auto& var : playerVars)
			processPlayerVar(playerVarsMem, var);

		gotPlayerVars = true;
	}
	void PlayerVariables::SortPlayerVars() {
		if (!playerVars.empty())
			return;

		std::stringstream ss(Config::playerVars);

		while (ss.good()) {
			// separate the string by the , character to get each variable
			std::string pVar{};
			getline(ss, pVar, ',');

			std::stringstream ssPVar(pVar);

			std::string varName{};
			std::string varType{};

			while (ssPVar.good()) {
				// seperate the string by the : character to get name and type of variable
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
			if (!Offsets::Get_PlayerState())
				return nullptr;

			PlayerState* ptr = *reinterpret_cast<PlayerState**>(Offsets::Get_PlayerState());
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
			if (!*reinterpret_cast<LPVOID*>(pPlayerHealthModule))
				return nullptr;

			return pPlayerHealthModule;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			pPlayerHealthModule = nullptr;
			return nullptr;
		}
	}
#pragma endregion

#pragma region TPPCameraDI
	TPPCameraDI* TPPCameraDI::Get() {
		__try {
			FreeCamera* pFreeCam = FreeCamera::Get();
			if (!pFreeCam)
				return nullptr;

			CoBaseCameraProxy* pCoBaseCameraProxy = pFreeCam->pCoBaseCameraProxy;
			if (!pCoBaseCameraProxy)
				return nullptr;

			TPPCameraDI* ptr = pCoBaseCameraProxy->pTPPCameraDI;
			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
#pragma endregion

#pragma region CameraFPPDI
	Vector3* CameraFPPDI::GetForwardVector(Vector3* outForwardVec) {
		Vector3* (*pGetForwardVector)(LPVOID pCameraFPPDI, Vector3 * outForwardVec) = (decltype(pGetForwardVector))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetForwardVector@IBaseCamera@@QEBA?BVvec3@@XZ");
		if (!pGetForwardVector)
			return nullptr;

		return pGetForwardVector(this, outForwardVec);
	}
	Vector3* CameraFPPDI::GetPosition(Vector3* posIN) {
		return Memory::CallVT<181, Vector3*>(this, posIN);
	}

	/*CameraFPPDI* CameraFPPDI::Get() {
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
	}*/
#pragma endregion

#pragma region FreeCamera
	Vector3* FreeCamera::GetForwardVector(Vector3* outForwardVec) {
		Vector3* (*pGetForwardVector)(LPVOID pFreeCamera, Vector3 * outForwardVec) = (decltype(pGetForwardVector))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetForwardVector@IBaseCamera@@QEBA?BVvec3@@XZ");
		if (!pGetForwardVector)
			return nullptr;

		return pGetForwardVector(this, outForwardVec);
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
			void(*pSetForcedWeather)(LPVOID timeWeatherSystem, int weather) = (decltype(pSetForcedWeather))Utils::GetProcAddr("engine_x64_rwdi.dll", "?SetForcedWeather@CSystem@TimeWeather@@QEAAXW4TYPE@EWeather@@VApiDebugAccess@2@@Z");
			if (!pSetForcedWeather)
				return;

			pSetForcedWeather(this, weather);
		}
		int CSystem::GetCurrentWeather() {
			int(*pGetCurrentWeather)(LPVOID timeWeatherSystem) = (decltype(pGetCurrentWeather))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetCurrentWeather@CSystem@TimeWeather@@QEBA?AW4TYPE@EWeather@@XZ");
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
	bool LevelDI::IsLoading() {
		bool(*pIsLoading)(LPVOID iLevel) = (decltype(pIsLoading))Utils::GetProcAddr("engine_x64_rwdi.dll", "?IsLoading@ILevel@@QEBA_NXZ");
		if (!pIsLoading)
			return true;

		return pIsLoading(this);
	}
	bool LevelDI::IsLoaded() {
		static float loadDeltaTime = 0.0f;
		if (IsLoading() || !GamePH::PlayerObjProperties::Get()) {
			loadDeltaTime = 0.0f;
			return false;
		}

		loadDeltaTime += GetTimeDelta();
		if (loadDeltaTime > 3.0f)
			return true;

		return false;
	}
	LPVOID LevelDI::GetViewCamera() {
		LPVOID(*pGetViewCamera)(LPVOID iLevel) = (decltype(pGetViewCamera))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetViewCamera@ILevel@@QEBAPEAVIBaseCamera@@XZ");
		if (!pGetViewCamera)
			return nullptr;

		return pGetViewCamera(this);
	}
	float LevelDI::GetTimeDelta() {
		return Memory::CallVT<176, float>(this);
	}
	void LevelDI::SetViewCamera(LPVOID viewCam) {
		Memory::CallVT<289, void>(this, viewCam);
	}
	float LevelDI::GetTimePlayed() {
		return Memory::CallVT<317, float>(this);
	}
	void LevelDI::ShowUIManager(bool enabled) {
		void(*pShowUIManager)(LPVOID iLevel, bool enabled) = (decltype(pShowUIManager))Utils::GetProcAddr("engine_x64_rwdi.dll", "?ShowUIManager@ILevel@@QEAAX_N@Z");
		if (!pShowUIManager)
			return;

		pShowUIManager(this, enabled);
	}
	TimeWeather::CSystem* LevelDI::GetTimeWeatherSystem() {
		TimeWeather::CSystem*(*pGetTimeWeatherSystem)(LevelDI* iLevel) = (decltype(pGetTimeWeatherSystem))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetTimeWeatherSystem@ILevel@@QEBAPEAVCSystem@TimeWeather@@XZ");
		if (!pGetTimeWeatherSystem)
			return nullptr;

		return pGetTimeWeatherSystem(this);
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

#pragma region gen_TPPModel
	gen_TPPModel* gen_TPPModel::Get() {
		__try {
			LocalClientDI* pLocalClientDI = LocalClientDI::Get();
			if (!pLocalClientDI)
				return nullptr;

			gen_TPPModel* ptr = pLocalClientDI->pgen_TPPModel;
			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
#pragma endregion

#pragma region LocalClientDI
	LocalClientDI* LocalClientDI::Get() {
		__try {
			SessionCooperativeDI* pSessionCooperativeDI = SessionCooperativeDI::Get();
			if (!pSessionCooperativeDI)
				return nullptr;

			LocalClientDI* ptr = pSessionCooperativeDI->pLocalClientDI;
			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
#pragma endregion

#pragma region SessionCooperativeDI
	SessionCooperativeDI* SessionCooperativeDI::Get() {
		__try {
			GameDI_PH* pGameDI_PH = GameDI_PH::Get();
			if (!pGameDI_PH)
				return nullptr;

			SessionCooperativeDI* ptr = pGameDI_PH->pSessionCooperativeDI;
			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
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
	float GameDI_PH::GetGameTimeDelta() {
		float(*pGetGameTimeDelta)(LPVOID pGameDI_PH) = (decltype(pGetGameTimeDelta))Utils::GetProcAddr("engine_x64_rwdi.dll", "?GetGameTimeDelta@IGame@@QEBAMXZ");
		if (!pGetGameTimeDelta)
			return -1.0f;

		return pGetGameTimeDelta(this);
	}
	DWORD64 GameDI_PH::GetCurrentGameVersion() {
		return Memory::CallVT<225, DWORD64>(this);
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

#pragma region BackgroundModuleScreenController
	BackgroundModuleScreenController* BackgroundModuleScreenController::Get() {
		__try {
			if (!Offsets::Get_g_BackgroundModuleScreenController())
				return nullptr;

			BackgroundModuleScreenController* ptr = reinterpret_cast<BackgroundModuleScreenController*>(Offsets::Get_g_BackgroundModuleScreenController());
			if (!Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll")) 
				return nullptr;

			return ptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
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
			if (!Offsets::Get_CLobbySteam())
				return nullptr;

			CLobbySteam* ptr = *reinterpret_cast<CLobbySteam**>(Offsets::Get_CLobbySteam());
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

#pragma region CRTTIField
	DWORD64 CRTTIField::Get_float(CRTTI* crtti, float& out) {
		if (!Offsets::Get_CRTTIFieldTypedNative_Get_float())
			return 0;

		DWORD64(*pCRTTIFieldTypedNative_Get_float)(LPVOID pCRTTIFieldTypedNative, CRTTI* crtti, float& out) = (decltype(pCRTTIFieldTypedNative_Get_float))Offsets::Get_CRTTIFieldTypedNative_Get_float();
		if (!pCRTTIFieldTypedNative_Get_float)
			return 0;

		return pCRTTIFieldTypedNative_Get_float(this, crtti, out);
	}
#pragma endregion

#pragma region CRTTI
	CRTTIField* CRTTI::FindField(const char* name) {
		if (!Offsets::Get_CRTTI_FindField())
			return nullptr;

		CRTTIField*(*pCRTTI_FindField)(LPVOID pCRTTI, const char* name) = (decltype(pCRTTI_FindField))Offsets::Get_CRTTI_FindField();
		if (!pCRTTI_FindField)
			return nullptr;

		return pCRTTI_FindField(this, name);
	}
#pragma endregion
}
#pragma endregion