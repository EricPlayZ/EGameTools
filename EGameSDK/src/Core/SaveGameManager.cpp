#include <spdlog\spdlog.h>
#include <EGSDK\Utils\Files.h>
#include <EGSDK\Core\Core.h>
#include <EGSDK\Core\SaveGameManager.h>
#include <EGSDK\Core\SteamAPI.h>

namespace EGSDK::Core {
    std::filesystem::path SaveGameManager::saveGamePath{};
    std::filesystem::path SaveGameManager::backupRootPath{};
    int SaveGameManager::maxBackupSlots = 16;
    std::chrono::minutes SaveGameManager::backupInterval{ 20 };
    int SaveGameManager::currentBackupSlot = 0;

    Utils::Time::Timer SaveGameManager::timeSpentInitializing{ 30000 };
    bool SaveGameManager::initialized = false;
    bool SaveGameManager::isInitializing = false;
    bool SaveGameManager::triedInitializingWithSteam = false;
    bool SaveGameManager::loopIsRunning = false;

    void SaveGameManager::Init() {
        if (initialized || isInitializing)
            return;
        isInitializing = true;
        timeSpentInitializing.Reset();

        while (true) {
            if (timeSpentInitializing.DidTimePass()) {
                if (!triedInitializingWithSteam) {
                    triedInitializingWithSteam = true;
                    timeSpentInitializing.Reset();
                    continue;
                }
                SPDLOG_ERROR("Failed initializing SaveGameManager after 20 seconds");
                MessageBoxA(nullptr, "EGameSDK encountered an issue trying to initialize its Savegame Manager.\nPLEASE MANUALLY MAKE BACKUPS OF YOUR SAVEGAME BEFORE using EGameSDK and any other mods, such as EGameTools!", "Failed initializing SaveGameManager", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
                return;
            }

            if (backupRootPath.empty()) {
                SPDLOG_INFO("Getting SDK storage path");
                std::filesystem::path sdkStoragePath = GetSDKStoragePath();
                if (!sdkStoragePath.empty())
                    backupRootPath = sdkStoragePath / "SavegameBackups";
            }
            if (saveGamePath.empty()) {
                SPDLOG_INFO("Detecting savegame path");
                saveGamePath = !triedInitializingWithSteam ? DetectSaveGamePathSteam() : DetectSaveGamePathEpic();
            }
            
            if (!saveGamePath.empty() && !backupRootPath.empty())
                break;

            Sleep(2000);
        }

        SPDLOG_INFO("SaveGameManager initialization successful");
        isInitializing = false;
        initialized = true;
    }

    void SaveGameManager::Loop() {
        if (!initialized || loopIsRunning)
            return;
        loopIsRunning = true;

        SPDLOG_INFO("Initializing backup slot");
        InitializeBackupSlot();

        SPDLOG_INFO("Performing first savegame backup...");
        PerformBackup();

        while (true) {
            std::this_thread::sleep_for(backupInterval);
            SPDLOG_INFO("Performing periodic savegame backup...");
            PerformBackup();
        }
    }

    void SaveGameManager::PerformBackup() {
        std::filesystem::path destinationPath = backupRootPath / ("Backup_" + std::to_string(currentBackupSlot));
        if (triedInitializingWithSteam)
            destinationPath = destinationPath / "out" / "storage";

        try {
            if (std::filesystem::exists(destinationPath))
                std::filesystem::remove_all(destinationPath);

            std::filesystem::create_directories(destinationPath);
            std::filesystem::copy(saveGamePath, destinationPath, std::filesystem::copy_options::recursive);

            SPDLOG_INFO("Backup successfully created in \"{}\"!", destinationPath.string());

            // Calculate next backup slot and cleanup
            int slotToDelete = (currentBackupSlot + 85) % 100;
            std::filesystem::path deletePath = backupRootPath / ("Backup_" + std::to_string(slotToDelete));
            if (std::filesystem::exists(deletePath)) {
                std::filesystem::remove_all(deletePath);
                SPDLOG_INFO("Deleted old backup folder: \"{}\"", deletePath.string());
            }
            currentBackupSlot = (currentBackupSlot + 1) % 100;
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("Exception caught while trying to backup savegame: {}", ex.what());
        }
    }

    void SaveGameManager::InitializeBackupSlot() {
        int highestSlot = -1;

        for (const auto& entry : std::filesystem::directory_iterator(backupRootPath)) {
            if (entry.is_directory()) {
                std::string folderName = entry.path().filename().string();
                if (folderName.rfind("Backup_", 0) == 0) { // Check if it starts with "Backup_"
                    try {
                        int slotNumber = std::stoi(folderName.substr(7));
                        if (slotNumber > highestSlot)
                            highestSlot = slotNumber;
                    } catch (const std::exception& ex) {
                        SPDLOG_ERROR("Failed to parse backup folder name: \"{}\", exception: {}", folderName, ex.what());
                    }
                }
            }
        }

        if (highestSlot != -1) {
            currentBackupSlot = (highestSlot + 1) % 100;
            SPDLOG_INFO("Initialized current backup slot to: {}", currentBackupSlot);
        } else
            SPDLOG_INFO("No existing backups found. Starting with slot 0");
    }
    std::filesystem::path SaveGameManager::DetectSaveGamePathSteam() {
        auto steamUser = SteamAPI::GetISteamUser();
        if (!steamUser) {
            SPDLOG_ERROR("Failed getting Steam user");
            return {};
        }

        char userDataFolder[256];
        if (!steamUser->GetUserDataFolder(userDataFolder, sizeof(userDataFolder))) {
            SPDLOG_ERROR("Failed getting Steam user data path");
            return {};
        }

        std::filesystem::path gameUserDataPath = userDataFolder;
        if (gameUserDataPath.filename() == "local")
            gameUserDataPath = gameUserDataPath.parent_path() / "remote";

        if (!std::filesystem::exists(gameUserDataPath)) {
            SPDLOG_ERROR("Savegame path does not exist: {}", gameUserDataPath.string());
            return {};
        }

        return gameUserDataPath;
    }
    std::filesystem::path SaveGameManager::DetectSaveGamePathEpic() {
        std::filesystem::path documentsPath = Utils::Files::GetDocumentsDir();
        if (documentsPath.empty()) {
            SPDLOG_ERROR("Failed getting user Documents path");
            return {};
        }

        std::filesystem::path gameStoragePath = documentsPath / "dying light 2" / "out" / "storage";
        if (!std::filesystem::exists(gameStoragePath)) {
            SPDLOG_ERROR("Savegame path does not exist: {}", gameStoragePath.string());
            return {};
        }

        return gameStoragePath;
    }
}