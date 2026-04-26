#pragma once
#include <filesystem>

namespace EGT::Config::SettingsStore {
	struct InitResult {
		bool migratedFromLegacy{ false };
		bool createdFromDefaults{ false };
		bool loadedExistingSettings{ false };
	};

	InitResult Init();
	bool Save();
	bool ReloadFromDisk();
	bool HasInMemoryChanges();
	bool FileExists();
	const std::filesystem::path& GetSettingsFilePath();
	void EnsureDefaultsApplied();
}
