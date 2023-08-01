#include <Windows.h>
#include <iostream>
#include <thread>
#include "utils.h"
#include "ini.h"
#include "kiero.h"
#include "ImGui\impl\d3d11_impl.h"
#include "MinHook\include\MinHook.h"
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
		freopen_s(&f, "CONOUT$", "w", stdout);
		DisableConsoleQuickEdit();
	}
	void DisableConsole() {
		if (f) fclose(f);
		FreeConsole();
	}

	// Core
	static void HookRendererThread() {
		while (true) {
			Sleep(1000);

			if (kiero::init(kiero::RenderType::Auto) != kiero::Status::Success)
				continue;

			switch (kiero::getRenderType()) {
			case kiero::RenderType::D3D11:
				impl::d3d11::init();
				break;
			case kiero::RenderType::D3D12:
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

	static void OnUpdateFreeCam() {

	}

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

		// Hook renderer for ImGui
		std::thread(HookRendererThread).detach();

		GamePH::LoopHookCreatePlayerHealthModule();
		GamePH::LoopHookOnUpdate();
		GamePH::LoopHookCalculateFreeCamCollision();
		GamePH::LoopHookLifeSetHealth();

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}
}