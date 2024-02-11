#include <pch.h>
#include "config\config.h"
#include "core.h"
#include "game\GamePH\PlayerVariables.h"
#include "menu\menu.h"

#pragma region KeyBindOption
bool KeyBindOption::wasAnyKeyPressed = false;
bool KeyBindOption::scrolledMouseWheelUp = false;
bool KeyBindOption::scrolledMouseWheelDown = false;
#pragma endregion

namespace Core {
#pragma region Console
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
#pragma endregion

	// Core
	bool exiting = false;
	static bool createdConfigThread = false;

	int rendererAPI = 0;
	static void LoopHookRenderer() {
		while (true) {
			if (exiting)
				return;

			Sleep(1000);

			if (!rendererAPI)
				continue;
			if (kiero::init(rendererAPI == 11 ? kiero::RenderType::D3D11 : kiero::RenderType::D3D12) != kiero::Status::Success)
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

	static void CreateSymlinkForLoadingFiles() {
		std::filesystem::create_directories("EGameTools\\FilesToLoad");

		for (const auto& entry : std::filesystem::directory_iterator("..\\..\\data")) {
			if (entry.path().filename().string() == "EGameTools" && is_symlink(entry.symlink_status())) {
				if (std::filesystem::equivalent("..\\..\\data\\EGameTools", "EGameTools"))
					return;
				std::filesystem::remove(entry.path());
			}
		}
		std::filesystem::create_directory_symlink(Utils::Files::GetCurrentProcDirectory() + "\\EGameTools", "..\\..\\data\\EGameTools");
	}

	void OnPostUpdate() {
		if (!createdConfigThread) {
			std::thread(Config::ConfigLoop).detach();
			std::thread(Config::ConfigSaveLoop).detach();

			createdConfigThread = true;
		}

		if (!GamePH::PlayerVariables::gotPlayerVars)
			GamePH::PlayerVariables::GetPlayerVars();

		for (auto& menuTab : *Menu::MenuTab::GetInstances())
			menuTab.second->Update();
	}
	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EnableConsole();

		Utils::PrintInfo("Initializing config");
		Config::InitConfig();
		Utils::PrintInfo("Creating \"EGameTools\\FilesToLoad\"");
		CreateSymlinkForLoadingFiles();
		Utils::PrintInfo("Sorting Player Variables");
		GamePH::PlayerVariables::SortPlayerVars();

		Utils::PrintInfo("Initializing MinHook");
		MH_Initialize();

		Utils::PrintInfo("Hooking DX11/DX12 renderer");
		std::thread([]() {
			LoopHookRenderer();
			Utils::PrintSuccess("Hooked DX11/DX12 renderer!");
		}).detach();

		for (auto& hook : *Utils::Hook::HookBase::GetInstances()) {
			Utils::PrintInfo("Hooking %s", hook->name.data());
			std::thread([hook]() { hook->HookLoop(); }).detach();
		}

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}

	void Cleanup() {
		exiting = true;

		Utils::PrintInfo("Game request exit, running cleanup");
		Utils::PrintInfo("Saving config to file");
		Config::SaveConfig();

		Utils::PrintInfo("Unhooking everything");
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
}