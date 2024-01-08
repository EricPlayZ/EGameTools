#include <Windows.h>
#include <iostream>
#include <thread>
#include <MinHook.h>
#include "kiero.h"
#include "ImGui\impl\d3d11_impl.h"
#include "ImGui\impl\d3d12_impl.h"
#include "menu\menu.h"
#include "menu\player.h"
#include "menu\camera.h"
#include "game_classes.h"
#include "sigscan\offsets.h"
#include "config\config.h"
#include "hook.h"
#include "utils.h"

namespace Core {
	// Console stuff
	static void DisableConsoleQuickEdit() {
		DWORD prev_mode = 0;
		const HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
		GetConsoleMode(hInput, &prev_mode);
		SetConsoleMode(hInput, prev_mode & ENABLE_EXTENDED_FLAGS);
	}

	static FILE* f = nullptr;
	static void EnableConsole() {
		AllocConsole();
		SetConsoleTitle("EGameTools");
		freopen_s(&f, "CONOUT$", "w", stdout);
		DisableConsoleQuickEdit();
	}
	void DisableConsole() {
		if (f)
			fclose(f);
		FreeConsole();
	}

	// Core
	bool exiting = false;
	static bool createdConfigThread = false;

	std::thread hookRendererThread{};
	std::thread configLoopThread{};
	std::thread configSaveLoopThread{};

	static std::string_view rendererAPI{};
	static void LoopHookRenderer() {
		while (true) {
			if (exiting)
				return;

			Sleep(1000);

			if (rendererAPI.empty())
				continue;

			if (kiero::init(rendererAPI == "d3d11" ? kiero::RenderType::D3D11 : kiero::RenderType::D3D12) != kiero::Status::Success)
				continue;

			switch (kiero::getRenderType()) {
			case kiero::RenderType::D3D11:
				impl::d3d11::init();
				break;
			case kiero::RenderType::D3D12:
				impl::d3d12::init();
				break;
			default:
				break;
			}

			break;
		}
	}

	#pragma region GetRendererAPI
	static bool(*pReadVideoSettings)(LPVOID instance, LPVOID file, bool flag1) = nullptr;
	static bool(*oReadVideoSettings)(LPVOID instance, LPVOID file, bool flag1) = nullptr;
	bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1) {
		if (!rendererAPI.empty())
			return oReadVideoSettings(instance, file, flag1);

		DWORD renderer = *reinterpret_cast<PDWORD>(reinterpret_cast<DWORD64>(instance) + 0x7C);
		rendererAPI = !renderer ? "d3d11" : "d3d12";
		
		return oReadVideoSettings(instance, file, flag1);
	}
	void LoopHookReadVideoSettings() {
		while (true) {
			Sleep(250);

			if (!pReadVideoSettings)
				pReadVideoSettings = (decltype(pReadVideoSettings))Offsets::Get_ReadVideoSettingsOffset();
			else if (!oReadVideoSettings && MH_CreateHook(pReadVideoSettings, &detourReadVideoSettings, reinterpret_cast<LPVOID*>(&oReadVideoSettings)) == MH_OK) {
				MH_EnableHook(pReadVideoSettings);
				break;
			}
		}
	}
	#pragma endregion

	void OnPostUpdate() {
		if (!createdConfigThread) {
			configLoopThread = std::thread(Config::ConfigLoop);
			configLoopThread.detach();
			configSaveLoopThread = std::thread(Config::ConfigSaveLoop);
			configSaveLoopThread.detach();

			createdConfigThread = true;
		}

		if (!GamePH::PlayerVariables::gotPlayerVars)
			GamePH::PlayerVariables::GetPlayerVars();

		Menu::Player::Update();
		Menu::Camera::Update();
	}

	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EnableConsole();

		Config::InitConfig();
		
		if (GamePH::PlayerVariables::playerVars.empty())
			GamePH::PlayerVariables::SortPlayerVars();

		MH_Initialize();
		LoopHookReadVideoSettings();

		hookRendererThread = std::thread(LoopHookRenderer);
		hookRendererThread.detach();

		GamePH::LoopHookCreatePlayerHealthModule();
		GamePH::LoopHookOnUpdate();
		GamePH::LoopHookCalculateFreeCamCollision();
		GamePH::LoopHookLifeSetHealth();
		GamePH::LoopHookTogglePhotoMode();
		GamePH::LoopHookMoveCamera();

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}

	void Cleanup() {
		exiting = true;

		Config::SaveConfig();

		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();

		configSaveLoopThread.join();
		configLoopThread.join();
		hookRendererThread.join();
	}
}