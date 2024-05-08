#include <pch.h>
#include "config\config.h"
#include "core.h"
#include "game\GamePH\LevelDI.h"
#include "game\GamePH\PlayerHealthModule.h"
#include "game\GamePH\PlayerInfectionModule.h"
#include "game\GamePH\PlayerVariables.h"
#include "game\GamePH\gameph_misc.h"
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

	int rendererAPI = 0;
	DWORD gameVer = 0;

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

	static bool WarnMsgSeenFileExists() {
		try {
			const std::string localAppDataDir = Utils::Files::GetLocalAppDataDir();
			if (localAppDataDir.empty())
				return false;
			const std::string finalPath = std::string(localAppDataDir) + "\\EGameTools\\" + "WarnMsgBoxSeen";

			return std::filesystem::exists(finalPath);
		} catch (const std::exception& e) {
			spdlog::error("Exception thrown while trying to check if WarnMsgBoxSeen file exists: {}", e.what());
			return false;
		}
	}
	static void CreateWarnMsgSeenFile() {
		try {
			const std::string localAppDataDir = Utils::Files::GetLocalAppDataDir();
			if (localAppDataDir.empty())
				return;
			const std::string dirPath = std::string(localAppDataDir) + "\\EGameTools\\";
			std::filesystem::create_directories(dirPath);

			const std::string finalPath = dirPath + "WarnMsgBoxSeen";
			if (!std::filesystem::exists(finalPath)) {
				std::ofstream outFile(finalPath.c_str(), std::ios::binary);
				if (!outFile.is_open())
					return;
				outFile.close();
			}
		} catch (const std::exception& e) {
			spdlog::error("Exception thrown while trying to create WarnMsgBoxSeen file: {}", e.what());
		}
	}
	static void CreateSymlinkForLoadingFiles() {
		try {
			const char* userModFilesPath = "..\\..\\..\\source\\data\\EGameTools\\UserModFiles";
			const char* EGameToolsPath = "..\\..\\..\\source\\data\\EGameTools";
			if (!std::filesystem::exists(userModFilesPath))
				std::filesystem::create_directories(userModFilesPath);

			for (const auto& entry : std::filesystem::directory_iterator(".")) {
				if (entry.path().filename().string() == "EGameTools") {
					if (is_symlink(entry.symlink_status()) && std::filesystem::equivalent("EGameTools", EGameToolsPath))
						return;
					
					std::filesystem::remove(entry.path());
				}
			}
			spdlog::warn("Creating folder shortcut \"EGameTools\" for \"Dying Light 2\\ph\\source\\data\\EGameTools\" folder");
			std::filesystem::create_directory_symlink(EGameToolsPath, Utils::Files::GetCurrentProcDirectory() + "\\EGameTools");
			spdlog::info("Game shortcut created");
		} catch (const std::exception& e) {
			spdlog::error("Exception thrown while trying to create folder shortcut: {}", e.what());
			spdlog::warn("This error should NOT affect any features of my mod. The shortcut is only a way for the user to easily access the folder \"Dying Light 2\\ph\\source\\data\\EGameTools\".");

			if (WarnMsgSeenFileExists())
				return;

			std::thread([]() {
				int msgBoxResult = MessageBoxA(nullptr, "EGameTools has failed creating a folder shortcut \"EGameTools\" inside \"Dying Light 2\\ph\\work\\bin\\x64\".\n\nTo fix this, please open Windows Settings and, for Windows 11, go to System -> For developers and enable \"Developer Mode\", or for Windows 10, go to Update & Security -> For developers and enable \"Developer Mode\".\nAfter doing this, restart the game and there should be no issues with shortcut creation anymore.\n\nIf the above solution doesn't work, then in order to install mods inside the \"UserModFiles\" folder, please manually navigate to \"Dying Light 2\\ph\\source\\data\\EGameTools\\UserModFiles\".\n\nAlternatively, run the game once as administrator from the exe and once a shortcut has been created, close the game and open up the game from Steam or whatever platform you're using.\n\nDo you want to continue seeing this warning message every game launch?", "Error creating EGameTools folder shortcut", MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND);

				switch (msgBoxResult) {
				case IDNO:
					CreateWarnMsgSeenFile();
					break;
				default:
					break;
				}
			}).detach();
		}
	}

	static void InitLogger() {
		constexpr size_t maxSize = static_cast<size_t>(1048576) * 100;
		constexpr size_t maxFiles = 10;

		try {
			static std::vector<spdlog::sink_ptr> sinks{};
			sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("..\\..\\..\\source\\data\\EGameTools\\log.txt", maxSize, maxFiles, true));
			sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
			std::shared_ptr<spdlog::logger> combined_logger = std::make_shared<spdlog::logger>("EGameTools", std::begin(sinks), std::end(sinks));
			combined_logger->flush_on(spdlog::level::trace);

			spdlog::set_default_logger(combined_logger);
		} catch (const std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			std::cout << "Failed creating spdlog instance! Please contact mod author, this is not supposed to happen!" << std::endl;
			std::cout << "Game will exit in 30 seconds. Close this window once you've read the text." << std::endl;
			Sleep(30000);
			exit(0);
		}
	}

	void OnPostUpdate() {
		GamePH::PlayerVariables::GetPlayerVars();

		for (auto& menuTab : *Menu::MenuTab::GetInstances())
			menuTab.second->Update();

		GamePH::PlayerHealthModule::UpdateClassAddr();
		GamePH::PlayerInfectionModule::UpdateClassAddr();
	}
	/*static bool WriteMiniDump(PEXCEPTION_POINTERS pExceptionPointers) {
		HANDLE hFile = CreateFileA("EGameTools-dump.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		MINIDUMP_EXCEPTION_INFORMATION mdei{};
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pExceptionPointers;
		mdei.ClientPointers = false;

		int success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, nullptr, nullptr);
		CloseHandle(hFile);

		return success;
	}
	static long WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo) {
		spdlog::error("VEH threw an exception with code {}. Trying to continue execution, writing mini-dump in the mean time.", ExceptionInfo->ExceptionRecord->ExceptionCode);

		if (WriteMiniDump(ExceptionInfo))
			spdlog::info("Mini-dump written to \"EGameTools-dump.dmp\". Please send this to mod author for further help!");
		else
			spdlog::error("Failed to write mini-dump.");

		return EXCEPTION_CONTINUE_EXECUTION;
	}*/
	static void GameVersionCheck() {
		try {
			gameVer = GamePH::GetCurrentGameVersion();
		} catch (const std::exception& e) {
			spdlog::error("Failed to get game version, EXCEPTION: {}", e.what());
			spdlog::error("This shouldn't happen! Contact developer.");
			return;
		}

		spdlog::info("Got game version: v{}", GamePH::GameVerToStr(gameVer));
		if (Core::gameVer != GAME_VER_COMPAT) {
			spdlog::error("Please note that your game version has not been officially tested with this mod, therefore expect bugs, glitches or the mod to completely stop working. If so, please {}", Core::gameVer > GAME_VER_COMPAT ? "wait for a new patch." : "upgrade your game version to one that the mod supports.");
		}
	}
	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EnableConsole();
		InitLogger();

		//AddVectoredExceptionHandler(0, &VectoredExceptionHandler);

		spdlog::warn("Getting game version");
		GameVersionCheck();

		spdlog::warn("Initializing config");
		Config::InitConfig();
		std::thread(Config::ConfigLoop).detach();

		CreateSymlinkForLoadingFiles();

		for (auto& hook : *Utils::Hook::HookBase::GetInstances()) {
			spdlog::warn("Hooking \"{}\"", hook->name.data());
			std::thread([&hook]() {
				if (hook->HookLoop())
					spdlog::info("Hooked \"{}\"!", hook->name.data());
			}).detach();
		}

		spdlog::warn("Sorting Player Variables");
		std::thread([]() {
			GamePH::PlayerVariables::SortPlayerVars();
			spdlog::info("Player Variables sorted");
		}).detach();

		spdlog::warn("Hooking DX11/DX12 renderer");
		std::thread([]() {
			LoopHookRenderer();
			spdlog::info("Hooked \"DX11/DX12 renderer\"!");
		}).detach();

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
