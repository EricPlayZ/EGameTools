#include <MinHook.h>
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <thread>
#include "ImGui\impl\d3d11_impl.h"
#include "ImGui\impl\d3d12_impl.h"
#include "config\config.h"
#include "game_classes.h"
#include "hook.h"
#include "kiero.h"
#include "menu\menu.h"
#include "print.h"
#include "sigscan\offsets.h"

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

	#pragma region ReadVideoSettings
	// forward decl
	static bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1);
	static Hook::MHook<LPVOID, bool(*)(LPVOID, LPVOID, bool)> ReadVideoSettingsHook{ "ReadVideoSettings", &Offsets::Get_ReadVideoSettings, &detourReadVideoSettings};

	static bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1) {
		if (!rendererAPI.empty())
			return ReadVideoSettingsHook.pOriginal(instance, file, flag1);

		DWORD renderer = *reinterpret_cast<PDWORD>(reinterpret_cast<DWORD64>(instance) + 0x7C);
		rendererAPI = !renderer ? "d3d11" : "d3d12";

		return ReadVideoSettingsHook.pOriginal(instance, file, flag1);
	}
	#pragma endregion

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

		/*Engine::CRTTI* g_BackgroundModuleScreenController = GamePH::BackgroundModuleScreenController::Get();
		if (!g_BackgroundModuleScreenController)
			return;

		Engine::CRTTIField* field = g_BackgroundModuleScreenController->FindField("m_LoadingProgress");
		if (!field)
			return;
		*/
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
		std::filesystem::create_directory_symlink(Utils::GetCurrentProcDirectory() + "\\EGameTools", "..\\..\\data\\EGameTools");
	}

	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EnableConsole();

		PrintInfo("Initializing config");
		Config::InitConfig();
		PrintInfo("Creating \"EGameTools\\FilesToLoad\"");
		CreateSymlinkForLoadingFiles();
		PrintInfo("Sorting Player Variables");
		GamePH::PlayerVariables::SortPlayerVars();

		PrintInfo("Initializing MinHook");
		MH_Initialize();

		PrintInfo("Hooking ReadVideoSettings");
		const auto readVidSettingsHook = std::find_if(Hook::HookBase::GetInstances()->begin(), Hook::HookBase::GetInstances()->end(), [](const auto& item) { return item->name == "ReadVideoSettings"; });
		if (readVidSettingsHook != Hook::HookBase::GetInstances()->end()) {
			(*readVidSettingsHook)->HookLoop();
			Hook::HookBase::GetInstances()->erase(readVidSettingsHook);
			PrintSuccess("Hooked ReadVideoSettings!");
		}

		PrintInfo("Hooking DX11/DX12 renderer");
		LoopHookRenderer();
		PrintSuccess("Hooked DX11/DX12 renderer!");

		for (auto& hook : *Hook::HookBase::GetInstances()) {
			PrintInfo("Hooking %s", hook->name.data());
			hook->HookLoop();
			PrintSuccess("Hooked %s!", hook->name.data());
		}

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}

	void Cleanup() {
		exiting = true;

		Config::SaveConfig();

		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
}