#include <pch.h>
#include "config\config.h"
#include "core.h"
#include "game\GamePH\LevelDI.h"
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
		std::filesystem::create_directories("EGameTools\\UserModFiles");

		for (const auto& entry : std::filesystem::directory_iterator("..\\..\\data")) {
			if (entry.path().filename().string() == "EGameTools" && is_symlink(entry.symlink_status())) {
				if (std::filesystem::equivalent("..\\..\\data\\EGameTools", "EGameTools"))
					return;
				std::filesystem::remove(entry.path());
			}
		}
		spdlog::warn("Creating game shortcut for \"EGameTools\"");
		std::filesystem::create_directory_symlink(Utils::Files::GetCurrentProcDirectory() + "\\EGameTools", "..\\..\\data\\EGameTools");
		spdlog::info("Game shortcut created");
	}

	static void InitLogger() {
		constexpr size_t maxSize = static_cast<size_t>(1048576) * 100;
		constexpr size_t maxFiles = 10;

		static std::vector<spdlog::sink_ptr> sinks{};
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("EGameTools\\log.txt", maxSize, maxFiles, true));
		sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
		std::shared_ptr<spdlog::logger> combined_logger = std::make_shared<spdlog::logger>("EGameTools", std::begin(sinks), std::end(sinks));
		combined_logger->flush_on(spdlog::level::trace);

		spdlog::set_default_logger(combined_logger);
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
		InitLogger();

		spdlog::warn("Initializing config");
		Config::InitConfig();
		CreateSymlinkForLoadingFiles();
		spdlog::warn("Sorting Player Variables");
		GamePH::PlayerVariables::SortPlayerVars();
		spdlog::info("Player Variables sorted");

		spdlog::warn("Initializing MinHook");
		MH_Initialize();
		spdlog::info("Initialized MinHook");

		spdlog::warn("Hooking DX11/DX12 renderer");
		std::thread([]() {
			LoopHookRenderer();
			spdlog::info("Hooked \"DX11/DX12 renderer\"!");
		}).detach();

		for (auto& hook : *Utils::Hook::HookBase::GetInstances()) {
			spdlog::warn("Hooking \"{}\"", hook->name.data());
			std::thread([&hook]() {
				hook->HookLoop();
				spdlog::info("Hooked \"{}\"!", hook->name.data());
			}).detach();
		}

		const HANDLE proc = GetCurrentProcess();
		WaitForSingleObject(proc, INFINITE);

		return TRUE;
	}

	void Cleanup() {
		exiting = true;

		spdlog::warn("Game requested exit, running cleanup");
		spdlog::warn("Saving config to file");
		Config::SaveConfig();
		spdlog::info("Config saved to file");

		spdlog::warn("Unhooking everything");
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
		spdlog::info("Unhooked everything");
	}
}