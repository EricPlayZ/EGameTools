#include <Windows.h>
#include <iostream>
#include <thread>
#include <MinHook.h>
#include "utils.h"
#include "ini.h"
#include "kiero.h"
#include "ImGui\impl\d3d11_impl.h"
#include "ImGui\impl\d3d12_impl.h"
#include "menu\menu.h"
#include "menu\player.h"
#include "menu\camera.h"
#include "game_classes.h"
#include "sigscan\offsets.h"
#include "hook.h"

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
		SetConsoleTitle("Debug Tools");
		freopen_s(&f, "CONOUT$", "w", stdout);
		DisableConsoleQuickEdit();
	}
	void DisableConsole() {
		if (f) fclose(f);
		FreeConsole();
	}

	// Core
	static std::string_view rendererAPI{};
	static void HookRendererThread() {
		while (true) {
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

	static std::unique_ptr<Hook::BreakpointHook> ldrpRoutineBpHook = nullptr;
	static void HookLdrpInitRoutine() {
		const WindowsVersion winVer = Utils::GetWindowsVersion();
		PDWORD64 pLdrpRoutineFunc = nullptr;

		if (winVer != WindowsVersion::Windows7)
			pLdrpRoutineFunc = Offsets::Get_LdrpCallInitRoutineOffset();
		else
			pLdrpRoutineFunc = Offsets::Get_LdrpRunInitializeRoutinesOffset();

		if (!pLdrpRoutineFunc)
			return;

		ldrpRoutineBpHook = std::make_unique<Hook::BreakpointHook>(pLdrpRoutineFunc, [&](PEXCEPTION_POINTERS info) -> void {
			DWORD64 entryPoint = info->ContextRecord->R12;

			if (Memory::IsValidPtrMod(entryPoint, "gamedll_ph_x64_rwdi.dll", false)) {
				ldrpRoutineBpHook->Disable();
				GamePH::PlayerVariables::RunHooks();
			}
		});
	}

	#pragma region GetRendererAPI
	static bool(*pReadVideoSettings)(LPVOID instance, LPVOID file, bool flag1) = nullptr;
	static bool(*oReadVideoSettings)(LPVOID instance, LPVOID file, bool flag1) = nullptr;
	bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1) {
		if (!rendererAPI.empty())
			return oReadVideoSettings(instance, file, flag1);

		DWORD renderer = *reinterpret_cast<PDWORD>(reinterpret_cast<DWORD64>(instance) + 0x78);
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
		if (!GamePH::PlayerVariables::gotPlayerVars)
			GamePH::PlayerVariables::GetPlayerVars();

		Menu::Player::Update();
		Menu::Camera::Update();

		//if (GamePH::GameDI_PH::Get()) {
		//	INT64 gameVer = GamePH::GameDI_PH::Get()->GetCurrentGameVersion();
		//	std::cout << "Game Version:" << gameVer << std::endl;
		//}
		//if (GamePH::LevelDI::Get()) {
		//	float timePlayed = GamePH::LevelDI::Get()->GetTimePlayed();
		//	std::cout << "Time Played: " << timePlayed << std::endl;
		//}
	}

	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EnableConsole();
		
		HookLdrpInitRoutine();

		MH_Initialize();
		LoopHookReadVideoSettings();

		// Hook renderer for ImGui
		std::thread(HookRendererThread).detach();

		GamePH::LoopHookCreatePlayerHealthModule();
		GamePH::LoopHookOnUpdate();
		GamePH::LoopHookCalculateFreeCamCollision();
		GamePH::LoopHookLifeSetHealth();
		GamePH::LoopHookTogglePhotoMode();

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}
}