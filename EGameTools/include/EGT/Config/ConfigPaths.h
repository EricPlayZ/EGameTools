#pragma once
#include <filesystem>

namespace EGT::Config::Paths {
	std::filesystem::path GetRuntimeDataDir();
	std::filesystem::path GetSourceDataDir();
	std::filesystem::path GetUserModFilesDir();
	std::filesystem::path GetSettingsFilePath();
	std::filesystem::path GetLegacyConfigFilePath();
	std::filesystem::path GetTeleportFilePath();
	std::filesystem::path GetMigrationReportPath();
	std::filesystem::path GetCrashDumpPath();
	std::filesystem::path GetGameRootDir();
}
