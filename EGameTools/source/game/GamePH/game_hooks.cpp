#include <pch.h>
#include "..\menu\camera.h"
#include "..\menu\misc.h"
#include "..\menu\player.h"
#include "..\menu\world.h"
#include "..\offsets.h"
#include "FreeCamera.h"
#include "GameDI_PH.h"
#include "GameDI_PH2.h"
#include "LevelDI.h"
#include "PlayerHealthModule.h"
#include "gen_TPPModel.h"

namespace GamePH {
	namespace Hooks {
#pragma region CreatePlayerHealthModule
		static DWORD64 detourCreatePlayerHealthModule(DWORD64 playerHealthModule);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64)> CreatePlayerHealthModuleHook{ "CreatePlayerHealthModule", &Offsets::Get_CreatePlayerHealthModule, &detourCreatePlayerHealthModule };

		static DWORD64 detourCreatePlayerHealthModule(DWORD64 playerHealthModule) {
			PlayerHealthModule::pPlayerHealthModule = reinterpret_cast<PlayerHealthModule*>(playerHealthModule);
			return CreatePlayerHealthModuleHook.pOriginal(playerHealthModule);
		}
#pragma endregion

#pragma region LifeSetHealth
		static void detourLifeSetHealth(float* pLifeHealth, float health);
		static Utils::Hook::MHook<LPVOID, void(*)(float*, float)> LifeSetHealthHook{ "LifeSetHealth", &Offsets::Get_LifeSetHealth, &detourLifeSetHealth };

		static void detourLifeSetHealth(float* pLifeHealth, float health) {
			if (!Menu::Player::godMode.GetValue() && !Menu::Camera::freeCam.GetValue())
				return LifeSetHealthHook.pOriginal(pLifeHealth, health);

			PlayerHealthModule* playerHealthModule = PlayerHealthModule::Get();
			if (!playerHealthModule)
				return LifeSetHealthHook.pOriginal(pLifeHealth, health);
			LevelDI* iLevel = LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return LifeSetHealthHook.pOriginal(pLifeHealth, health);

			if (std::abs(reinterpret_cast<LONG64>(playerHealthModule) - reinterpret_cast<LONG64>(pLifeHealth)) < 0x100 && playerHealthModule->health > 0.0f)
				return;

			LifeSetHealthHook.pOriginal(pLifeHealth, health);
		}
#pragma endregion

#pragma region IsNotOutOfBounds
		static bool detourIsNotOutOfBounds(LPVOID pInstance, DWORD64 a2);
		static Utils::Hook::MHook<LPVOID, bool(*)(LPVOID, DWORD64)> IsNotOutOfBoundsHook{ "IsNotOutOfBounds", &Offsets::Get_IsNotOutOfBounds, &detourIsNotOutOfBounds };

		static bool detourIsNotOutOfBounds(LPVOID pInstance, DWORD64 a2) {
			if (Menu::Player::disableOutOfBoundsTimer.GetValue())
				return true;

			return IsNotOutOfBoundsHook.pOriginal(pInstance, a2);
		}
#pragma endregion

#pragma region ShowUIManager
		static void detourShowUIManager(LPVOID pLevelDI, bool enabled);
		static LPVOID GetShowUIManager() {
			return Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?ShowUIManager@ILevel@@QEAAX_N@Z");
		}
		static Utils::Hook::MHook<LPVOID, void(*)(LPVOID, bool)> ShowUIManagerHook{ "ShowUIManager", &GetShowUIManager, &detourShowUIManager };

		static void detourShowUIManager(LPVOID pLevelDI, bool enabled) {
			if (Menu::Misc::disableHUD.GetValue())
				enabled = false;

			ShowUIManagerHook.pOriginal(pLevelDI, enabled);
		}
#pragma endregion

#pragma region OnPostUpdate
		static void detourOnPostUpdate(LPVOID pGameDI_PH2);
		static Utils::Hook::VTHook<GamePH::GameDI_PH2*, void(*)(LPVOID)> OnPostUpdateHook{ "OnPostUpdate", &GamePH::GameDI_PH2::Get, &detourOnPostUpdate, 0x3A8 };

		static void detourOnPostUpdate(LPVOID pGameDI_PH2) {
			OnPostUpdateHook.pOriginal(pGameDI_PH2);
			Core::OnPostUpdate();
		}
#pragma endregion

#pragma region TogglePhotoMode
		static void detourTogglePhotoMode1(LPVOID guiPhotoModeData, bool enabled);
		static Utils::Hook::MHook<LPVOID, void(*)(LPVOID, bool)> TogglePhotoMode1Hook{ "TogglePhotoMode1", &Offsets::Get_TogglePhotoMode1, &detourTogglePhotoMode1 };

		static void detourTogglePhotoMode1(LPVOID guiPhotoModeData, bool enabled) {
			Menu::Camera::photoMode.Set(enabled);

			if (!Menu::Camera::freeCam.GetValue())
				return TogglePhotoMode1Hook.pOriginal(guiPhotoModeData, enabled);
			LevelDI* iLevel = LevelDI::Get();
			if (!iLevel || iLevel->IsTimerFrozen())
				return TogglePhotoMode1Hook.pOriginal(guiPhotoModeData, enabled);
			GameDI_PH* pGameDI_PH = GameDI_PH::Get();
			if (!pGameDI_PH)
				return TogglePhotoMode1Hook.pOriginal(guiPhotoModeData, enabled);
			FreeCamera* pFreeCam = FreeCamera::Get();
			if (!pFreeCam)
				return TogglePhotoMode1Hook.pOriginal(guiPhotoModeData, enabled);

			if (enabled) {
				pGameDI_PH->TogglePhotoMode();
				pFreeCam->AllowCameraMovement(0);
			}

			TogglePhotoMode1Hook.pOriginal(guiPhotoModeData, enabled);
		}
#pragma endregion

#pragma region ShowTPPModelFunc3
		Option wannaUseTPPModel{};
		static bool prevUseTPPModel;
		static void detourShowTPPModelFunc3(DWORD64 tppFunc2Addr, bool showTPPModel);
		static Utils::Hook::MHook<LPVOID, void(*)(DWORD64, bool)> ShowTPPModelFunc3Hook{ "ShowTPPModelFunc3", &Offsets::Get_ShowTPPModelFunc3, &detourShowTPPModelFunc3 };

		static void detourShowTPPModelFunc3(DWORD64 tppFunc2Addr, bool showTPPModel) {
			gen_TPPModel* pgen_TPPModel = gen_TPPModel::Get();
			if (!pgen_TPPModel) {
				ShowTPPModelFunc3Hook.pOriginal(tppFunc2Addr, showTPPModel);
				return;
			}
			
			if (!showTPPModel && prevUseTPPModel) {
				pgen_TPPModel->enableTPPModel2 = true;
				pgen_TPPModel->enableTPPModel1 = true;
			}
			ShowTPPModelFunc3Hook.pOriginal(tppFunc2Addr, showTPPModel);
			if (showTPPModel && prevUseTPPModel) {
				pgen_TPPModel->enableTPPModel2 = false;
				pgen_TPPModel->enableTPPModel1 = false;
			} else
				prevUseTPPModel = showTPPModel;
		}
#pragma endregion

#pragma region CalculateFreeCamCollision
		static DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(LPVOID, float*)> CalculateFreeCamCollisionHook{ "CalculateFreeCamCollision", &Offsets::Get_CalculateFreeCamCollision, &detourCalculateFreeCamCollision };

		static DWORD64 detourCalculateFreeCamCollision(LPVOID pFreeCamera, float* finalPos) {
			if (Menu::Camera::disablePhotoModeLimits.GetValue() || Menu::Camera::freeCam.GetValue())
				return 0;

			return CalculateFreeCamCollisionHook.pOriginal(pFreeCamera, finalPos);
		}
#pragma endregion

#pragma region PlaySoundEvent
		static DWORD64 detourPlaySoundEvent(LPVOID pCoAudioEventControl, DWORD64 name, DWORD64 a3);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(LPVOID, DWORD64, DWORD64)> PlaySoundEventHook{ "PlaySoundEvent", &Offsets::Get_PlaySoundEvent, &detourPlaySoundEvent };

		static DWORD64 detourPlaySoundEvent(LPVOID pCoAudioEventControl, DWORD64 name, DWORD64 a3) {
			const char* soundName = reinterpret_cast<const char*>(name & 0x1FFFFFFFFFFFFFFF); // remove first byte of addr in case it exists
			if (Menu::World::freezeTime.GetValue() && soundName &&
				(!strcmp(soundName, "set_gp_infection_start") || !strcmp(soundName, "set_gp_infection_immune"))) {
				return 0;
			}

			return PlaySoundEventHook.pOriginal(pCoAudioEventControl, name, a3);
		}
#pragma endregion

#pragma region CalculateFallHeight
		static DWORD64 detourCalculateFallHeight(LPVOID pInstance, float height);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(LPVOID, float)> CalculateFallHeightHook{ "CalculateFallHeight", &Offsets::Get_CalculateFallHeight, &detourCalculateFallHeight };

		static DWORD64 detourCalculateFallHeight(LPVOID pInstance, float height) {
			static bool prevFreeCam = Menu::Camera::freeCam.GetPrevValue();
			prevFreeCam = Menu::Camera::freeCam.GetPrevValue();
			if (!Menu::Camera::freeCam.GetValue() && prevFreeCam) {
				Menu::Camera::freeCam.SetPrevValue(false);
				prevFreeCam = false;
				return 0;
			}

			return CalculateFallHeightHook.pOriginal(pInstance, height);
		}
#pragma endregion
	}
}