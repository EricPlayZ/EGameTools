#include <spdlog\spdlog.h>
#include <spdlog\sinks\rotating_file_sink.h>
#include <EGSDK\Core\Core.h>
#include <EGSDK\Utils\Files.h>
#include <EGSDK\Utils\Hook.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\GamePH\PlayerHealthModule.h>
#include <EGSDK\GamePH\PlayerInfectionModule.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\GamePH\GamePH_Hooks.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGT\ImGui_impl\D3D11_impl.h>
#include <EGT\ImGui_impl\D3D12_impl.h>
#include <EGT\Config\Config.h>
#include <EGT\Config\ConfigPaths.h>
#include <EGT\Engine\Engine_Hooks.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Misc.h>
#include <EGT\Menu\Debug.h>
#include <DbgHelp.h>
#include <thread>
#include <semaphore>
#include <fstream>
#include <iostream>

namespace EGT::Core {
#pragma region Console
	static void DisableConsoleQuickEdit() {
		DWORD prev_mode = 0;
		const HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
		GetConsoleMode(hInput, &prev_mode);
		SetConsoleMode(hInput, prev_mode & ENABLE_EXTENDED_FLAGS);
	}

	static FILE* f = nullptr;
	void OpenIOBuffer() {
		freopen_s(&f, "CONOUT$", "w", stdout);
	}
	void CloseIOBuffer() {
		if (f)
			fclose(f);
	}
    void EnableConsole() {
        AllocConsole();
        SetConsoleTitle("EGameTools");
        HWND consoleWindow = GetConsoleWindow();
        MoveWindow(consoleWindow, 0, 0, 1800, 720, TRUE);
        DisableConsoleQuickEdit();
    }
	void DisableConsole() {
		FreeConsole();
	}
#pragma endregion

	// Core
	std::atomic<bool> exiting = false;
	std::counting_semaphore<4> maxHookThreads(4);
	static std::vector<std::thread> threads{};
	static HANDLE keepAliveEvent{};

	static bool LoopHookRenderer() {
		SPDLOG_INFO("Entering LoopHookRenderer loop");
		while (true) {
			if (exiting) {
				SPDLOG_INFO("Exiting LoopHookRenderer loop due to exiting flag");
				return false;
			}

			Sleep(1000);

			if (!EGSDK::Core::rendererAPI) {
				auto d3d11Module = GetModuleHandle("rd3d11_x64_rwdi.dll");
				auto d3d12Module = GetModuleHandle("rd3d12_x64_rwdi.dll");

				EGSDK::Core::rendererAPI = d3d12Module ? 12 : d3d11Module ? 11 : 0;
				SPDLOG_INFO("rendererAPI is null, skipping iteration");
				continue;
			}
			switch (EGSDK::Core::rendererAPI) {
				case 11:
					SPDLOG_INFO("Initializing D3D11 ImGui implementation");
					EGT::ImGui_impl::D3D11::Init();
					break;
				case 12:
					SPDLOG_INFO("Initializing D3D12 ImGui implementation");
					EGT::ImGui_impl::D3D12::Init();
					break;
				default:
					SPDLOG_ERROR("Unknown renderer type");
					break;
			}

			break;
		}
		SPDLOG_INFO("Exiting LoopHookRenderer loop normally");
		return true;
	}

	static bool WarnMsgSeenFileExists() {
		SPDLOG_DEBUG("Checking if WarnMsgBoxSeen file exists");
		try {
			const std::string localAppDataDir = EGSDK::Utils::Files::GetLocalAppDataDir();
			SPDLOG_DEBUG("Local App Data Dir: {}", localAppDataDir);
			if (localAppDataDir.empty()) {
				SPDLOG_WARN("Local App Data Dir is empty");
				return false;
			}
			const std::string finalPath = std::string(localAppDataDir) + "\\EGameTools\\" + "WarnMsgBoxSeen";
			SPDLOG_DEBUG("Final Path: {}", finalPath);

			bool exists = std::filesystem::exists(finalPath);
			SPDLOG_DEBUG("WarnMsgBoxSeen file exists: {}", exists);
			return exists;
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Exception thrown while trying to check if WarnMsgBoxSeen file exists: {}", e.what());
			return false;
		}
	}
	static void CreateWarnMsgSeenFile() {
		SPDLOG_INFO("Creating WarnMsgBoxSeen file...");
		try {
			const std::string localAppDataDir = EGSDK::Utils::Files::GetLocalAppDataDir();
			SPDLOG_DEBUG("Local App Data Dir: {}", localAppDataDir);
			if (localAppDataDir.empty()) {
				SPDLOG_WARN("Local App Data Dir is empty, skipping WarnMsgBoxSeen file creation");
				return;
			}
			const std::string dirPath = std::string(localAppDataDir) + "\\EGameTools\\";
			SPDLOG_DEBUG("Directory Path: {}", dirPath);
			std::filesystem::create_directories(dirPath);
			SPDLOG_INFO("Created directory: {}", dirPath);

			const std::string finalPath = dirPath + "WarnMsgBoxSeen";
			SPDLOG_DEBUG("Final Path: {}", finalPath);
			if (!std::filesystem::exists(finalPath)) {
				SPDLOG_INFO("Creating WarnMsgBoxSeen file...");
				std::ofstream outFile(finalPath.c_str(), std::ios::binary);
				if (!outFile.is_open()) {
					SPDLOG_ERROR("Failed to open WarnMsgBoxSeen file for writing");
					return;
				}
				outFile.close();
				SPDLOG_INFO("WarnMsgBoxSeen file created successfully");
			}
			SPDLOG_INFO("WarnMsgBoxSeen file already exists, skipping creation");
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Exception thrown while trying to create WarnMsgBoxSeen file: {}", e.what());
		}
	}
	static void CreateSymlinkForLoadingFiles() {
		SPDLOG_DEBUG("Entering CreateSymlinkForLoadingFiles");
		try {
			const std::filesystem::path userModFilesPath = Config::Paths::GetUserModFilesDir();
			const std::filesystem::path eGameToolsSourcePath = Config::Paths::GetSourceDataDir();
			const std::filesystem::path runtimeDir = EGSDK::Utils::Files::GetCurrentProcDirectory();
			const std::filesystem::path sourceShortcutPath = runtimeDir / "EGameToolsSource";
			const std::filesystem::path legacyShortcutPath = runtimeDir / "EGameTools";

			SPDLOG_DEBUG("UserModFilesPath: {}", userModFilesPath.string());
			SPDLOG_DEBUG("EGameToolsSourcePath: {}", eGameToolsSourcePath.string());

			if (!std::filesystem::exists(userModFilesPath)) {
				SPDLOG_DEBUG("UserModFilesPath does not exist, creating directories");
				std::filesystem::create_directories(userModFilesPath);
				SPDLOG_INFO("Created directories: {}", userModFilesPath.string());
			} else {
				SPDLOG_DEBUG("UserModFilesPath already exists");
			}

			// Migrate old shortcut name to the new one.
			if (std::filesystem::exists(legacyShortcutPath) && !std::filesystem::exists(sourceShortcutPath)) {
				try {
					if (std::filesystem::is_symlink(legacyShortcutPath) && std::filesystem::equivalent(legacyShortcutPath, eGameToolsSourcePath)) {
						std::filesystem::rename(legacyShortcutPath, sourceShortcutPath);
						SPDLOG_INFO("Migrated legacy source-data shortcut from {} to {}", legacyShortcutPath.string(), sourceShortcutPath.string());
					}
				} catch (const std::exception& e) {
					SPDLOG_WARN("Failed migrating legacy source-data shortcut {}: {}", legacyShortcutPath.string(), e.what());
				}
			}

			// Remove legacy shortcut if the new one already exists.
			if (std::filesystem::exists(legacyShortcutPath) && std::filesystem::exists(sourceShortcutPath)) {
				try {
					if (std::filesystem::is_symlink(legacyShortcutPath))
						std::filesystem::remove(legacyShortcutPath);
				} catch (const std::exception& e) {
					SPDLOG_WARN("Failed removing legacy source-data shortcut {}: {}", legacyShortcutPath.string(), e.what());
				}
			}

			for (const auto& entry : std::filesystem::directory_iterator(runtimeDir)) {
				SPDLOG_DEBUG("Iterating directory entry: {}", entry.path().filename().string());

				if (entry.path().filename().string() == sourceShortcutPath.filename().string()) {
					SPDLOG_DEBUG("Found existing source shortcut path");

					if (is_symlink(entry.symlink_status()) && std::filesystem::equivalent(sourceShortcutPath, eGameToolsSourcePath)) {
						SPDLOG_DEBUG("EGameToolsSource is already a valid symlink, returning");
						return;
					}

					SPDLOG_DEBUG("Removing stale source shortcut path");
					std::filesystem::remove(entry.path());
					SPDLOG_INFO("Removed path: {}", entry.path().filename().string());
				}
			}

			SPDLOG_INFO("Creating optional source-data shortcut \"EGameToolsSource\" for source\\data\\EGameTools");
			std::filesystem::create_directory_symlink(eGameToolsSourcePath, sourceShortcutPath);
			SPDLOG_INFO("Game source-data shortcut created: {}", sourceShortcutPath.string());
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Exception thrown while trying to create folder shortcut: {}", e.what());
			SPDLOG_WARN("This error should NOT affect any features. The shortcut is only a convenience for opening \"Dying Light 2\\ph\\source\\data\\EGameTools\".");

			if (WarnMsgSeenFileExists()) {
				SPDLOG_DEBUG("WarnMsgSeenFile already exists, returning");
				return;
			}

			std::thread([]() {
				int msgBoxResult = MessageBoxA(nullptr, "EGameTools failed creating the optional folder shortcut \"EGameToolsSource\" inside \"Dying Light 2\\ph\\work\\bin\\x64\".\n\nTo fix this, please open Windows Settings and, for Windows 11, go to System -> For developers and enable \"Developer Mode\", or for Windows 10, go to Update & Security -> For developers and enable \"Developer Mode\".\nAfter doing this, restart the game and there should be no issues with shortcut creation anymore.\n\nEven if this fails, all features will still work. For manually installing mods, use \"Dying Light 2\\ph\\source\\data\\EGameTools\\UserModFiles\".\n\nDo you want to continue seeing this warning message every game launch?", "Error creating EGameTools source shortcut", MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND);

				switch (msgBoxResult) {
				case IDNO:
					SPDLOG_DEBUG("User chose to not see warning message again");
					CreateWarnMsgSeenFile();
					break;
				default:
					SPDLOG_DEBUG("User chose to see warning message again");
					break;
				}
			}).detach();
		}
		SPDLOG_DEBUG("Exiting CreateSymlinkForLoadingFiles");
	}

	void InitLogger() {
		try {
			std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("EGameTools", std::begin(EGSDK::Core::spdlogSinks), std::end(EGSDK::Core::spdlogSinks));

			EGSDK::Core::SetDefaultLoggerSettings(logger);
			spdlog::set_default_logger(logger);
		} catch (const std::exception& e) {
			UNREFERENCED_PARAMETER(e);
			std::string errorMsg = "Failed creating spdlog instance, EXCEPTION: " + std::string(e.what()) + "\n\nThis shouldn't happen! Contact developer.";
			MessageBoxA(nullptr, errorMsg.c_str(), "FATAL GAME ERROR", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
			exit(0);
		}
	}
	static void AddConsoleSink() {
		EGSDK::Core::spdlogSinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
		InitLogger();
	}

	static void OnPostUpdate(void* pGameDI_PH2) {
		for (auto& menuTab : *Menu::MenuTab::GetInstances()) {
			if (!EGSDK::GamePH::Hooks::didOnPostUpdateHookExecute)
				menuTab.second->Init();
			menuTab.second->Update();
		}

		static bool mountDataPaksErrorShown = false;
		if (!mountDataPaksErrorShown && EGT::Engine::Hooks::mountDataPaksRanWith8Count < 3 && Menu::Misc::increaseDataPAKsLimit.GetValue() && EGSDK::GamePH::PlayerVariables::Get()) {
			SPDLOG_ERROR("MountDataPaks hook ran less than 3 times with the data PAKs limit set to 8. This means the increased data PAKs limit might not work correctly! If this error message appears and your data PAKs past \"data7.pak\" have not loaded, please contact author.");
			mountDataPaksErrorShown = true;
		}
	}
#ifndef EXCP_HANDLER_DISABLE_DEBUG
	static bool WriteMiniDump(PEXCEPTION_POINTERS pExceptionPointers) {
		const auto dumpPath = Config::Paths::GetCrashDumpPath().string();
		HANDLE hFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		MINIDUMP_EXCEPTION_INFORMATION mdei{};
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pExceptionPointers;
		mdei.ClientPointers = false;

		int success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, (pExceptionPointers ? &mdei : nullptr), nullptr, nullptr);
		CloseHandle(hFile);

		return success;
	}
	static long WINAPI CrashHandler(PEXCEPTION_POINTERS exceptionInfo) {
		SPDLOG_ERROR("Crash Handler threw an exception with code {}. Game is exiting, writing mini-dump in the mean time.", exceptionInfo->ExceptionRecord->ExceptionCode);
		std::string errorMsg = "";

		if (WriteMiniDump(exceptionInfo)) {
			SPDLOG_INFO("Mini-dump written to \"{}\". Please send this to mod author for further help!", Config::Paths::GetCrashDumpPath().string());
			errorMsg = "EGameTools encountered a fatal error that caused the game to crash.\n\nA file \"" + Config::Paths::GetCrashDumpPath().string() + "\" has been generated. Please send this file to the author of the mod!\n\nThe game will now close once you press OK.";
		} else {
			SPDLOG_ERROR("Failed to write mini-dump.");
			errorMsg = "EGameTools encountered a fatal error that caused the game to crash.\n\nEGameTools failed to generate a crash dump file unfortunately, which means it is harder to find the cause of the crash.\n\nThe game will now close once you press OK.";
		}

		MessageBoxA(nullptr, errorMsg.c_str(), "Fatal game error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
		exit(0);
	}
#endif
	DWORD64 WINAPI MainThread(HMODULE hModule) {
		EGSDK::GamePH::Hooks::OnPostUpdateHook.RegisterCallback(OnPostUpdate);

#ifndef EXCP_HANDLER_DISABLE_DEBUG
		SetUnhandledExceptionFilter(CrashHandler);
#endif

		SPDLOG_INFO("Initializing config");
		Config::InitConfig();
		threads.emplace_back(Config::ConfigLoop);

		if (Menu::Debug::enableDebuggingConsole.GetValue()) {
			OpenIOBuffer();
			EnableConsole();
			AddConsoleSink();
		}

		SPDLOG_INFO("Setting vftable scanning to: {}", Menu::Debug::disableVftableScanning.GetValue());
		EGSDK::ClassHelpers::SetIsVftableScanningDisabled(Menu::Debug::disableVftableScanning.GetValue());

		SPDLOG_INFO("Creating optional source-data shortcut for loading files");
		CreateSymlinkForLoadingFiles();

		SPDLOG_INFO("Initializing hooks");
		for (auto& hook : (*EGSDK::Utils::Hook::HookBase::GetInstances())[hModule]) {
			threads.emplace_back([&hook]() {
				maxHookThreads.acquire();

				if (hook->IsHooking()) {
					SPDLOG_INFO("Hooking \"{}\"", hook->GetName().data());
					while (hook->IsHooking())
						Sleep(10);

					if (hook->IsHooked())
						SPDLOG_INFO("Hooked \"{}\"!", hook->GetName().data());
				} else if (hook->IsHooked())
					SPDLOG_INFO("Hooked \"{}\"!", hook->GetName().data());
				else if (hook->CanHookOnStartup()) {
					SPDLOG_INFO("Hooking \"{}\"", hook->GetName().data());
					if (hook->TryHooking())
						SPDLOG_INFO("Hooked \"{}\"!", hook->GetName().data());
				}

				maxHookThreads.release();
			}).detach();
		}

		SPDLOG_INFO("Hooking DX11/DX12 renderer");
		threads.emplace_back([]() {
			if (LoopHookRenderer())
				SPDLOG_INFO("Hooked \"DX{} renderer\"!", EGSDK::Core::rendererAPI);
			else
				SPDLOG_ERROR("Failed to hook renderer");
		}).detach();

		SPDLOG_INFO("Creating keepAliveEvent");
		keepAliveEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
		if (!keepAliveEvent) {
			SPDLOG_ERROR("Failed to create keepAliveEvent");
			MessageBoxA(nullptr, "EGameTools encountered a fatal error: failed to create keepAliveEvent", "Fatal game error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
			exit(0);
		}
		WaitForSingleObject(keepAliveEvent, INFINITE);

		for (auto& thread : threads) {
			if (thread.joinable())
				thread.join();
		}

		return true;
	}

	void Cleanup() {
		exiting = true;

		SPDLOG_INFO("Game requested exit, running cleanup");
		SPDLOG_INFO("Saving config to file");
		Config::SaveConfig();
		SPDLOG_INFO("Config saved to file");

		SPDLOG_INFO("Unhooking everything");
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
		SPDLOG_INFO("Unhooked everything");

		SPDLOG_INFO("Disabling console");
		if (Menu::Debug::enableDebuggingConsole.GetValue()) {
			DisableConsole();
			CloseIOBuffer();
		}
		SPDLOG_INFO("Console disabled");

		SetEvent(keepAliveEvent);
	}
}
