#include <Windows.h>
#include <filesystem>
#include <spdlog\spdlog.h>
#include <EGT\Config\Config.h>
#include <EGT\Config\SettingsStore.h>
#include <EGT\Config\TeleportStore.h>
#include <EGT\Core\Core.h>

namespace EGT::Config {
	std::filesystem::file_time_type settingsPreviousWriteTime{};
	std::filesystem::file_time_type settingsLastWriteTime{};
	std::filesystem::file_time_type teleportsPreviousWriteTime{};
	std::filesystem::file_time_type teleportsLastWriteTime{};
	bool settingsSavedByApp = false;
	bool teleportsSavedByApp = false;

	void RefreshTrackedWriteTimes() {
		if (SettingsStore::FileExists()) {
			settingsPreviousWriteTime = std::filesystem::last_write_time(SettingsStore::GetSettingsFilePath());
			settingsLastWriteTime = settingsPreviousWriteTime;
		}
		if (TeleportStore::FileExists()) {
			teleportsPreviousWriteTime = std::filesystem::last_write_time(TeleportStore::GetTeleportFilePath());
			teleportsLastWriteTime = teleportsPreviousWriteTime;
		}
	}

	void SaveConfig() {
		if (SettingsStore::Save())
			settingsSavedByApp = true;

		if (TeleportStore::Save())
			teleportsSavedByApp = true;
	}

	void InitConfig() {
		const auto settingsInitResult = SettingsStore::Init();
		TeleportStore::Init();
		RefreshTrackedWriteTimes();

		if (settingsInitResult.migratedFromLegacy)
			SPDLOG_INFO("Config migration finished; now using settings.toml + teleport_locations.toml");
	}

	void ConfigLoop() {
		while (!Core::exiting) {
			Sleep(2000);

			if (!SettingsStore::FileExists()) {
				SettingsStore::Save();
				Sleep(300);
				RefreshTrackedWriteTimes();
				continue;
			}

			if (!TeleportStore::FileExists()) {
				TeleportStore::Save();
				Sleep(300);
				RefreshTrackedWriteTimes();
				continue;
			}

			settingsLastWriteTime = std::filesystem::last_write_time(SettingsStore::GetSettingsFilePath());
			if (settingsLastWriteTime != settingsPreviousWriteTime && !settingsSavedByApp) {
				settingsPreviousWriteTime = settingsLastWriteTime;
				SettingsStore::ReloadFromDisk();
			} else if (settingsLastWriteTime != settingsPreviousWriteTime && settingsSavedByApp) {
				settingsPreviousWriteTime = settingsLastWriteTime;
				settingsSavedByApp = false;
			}

			teleportsLastWriteTime = std::filesystem::last_write_time(TeleportStore::GetTeleportFilePath());
			if (teleportsLastWriteTime != teleportsPreviousWriteTime && !teleportsSavedByApp) {
				teleportsPreviousWriteTime = teleportsLastWriteTime;
				TeleportStore::ReloadFromDisk();
			} else if (teleportsLastWriteTime != teleportsPreviousWriteTime && teleportsSavedByApp) {
				teleportsPreviousWriteTime = teleportsLastWriteTime;
				teleportsSavedByApp = false;
			}

			if (SettingsStore::HasInMemoryChanges()) {
				if (SettingsStore::Save())
					settingsSavedByApp = true;
			}

			if (TeleportStore::HasInMemoryChanges()) {
				if (TeleportStore::Save())
					teleportsSavedByApp = true;
			}
		}
	}
}