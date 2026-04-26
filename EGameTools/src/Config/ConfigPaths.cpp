#include <filesystem>
#include <EGSDK\Utils\Files.h>
#include <EGT\Config\ConfigPaths.h>

namespace EGT::Config::Paths {
	std::filesystem::path Normalize(const std::filesystem::path& path) {
		try {
			return path.lexically_normal();
		} catch (...) {
			return path;
		}
	}

	std::filesystem::path GetRuntimeDataDir() {
		return Normalize(std::filesystem::path(EGSDK::Utils::Files::GetCurrentProcDirectory()) / "EGameTools");
	}

	std::filesystem::path GetSourceDataDir() {
		const auto runtimeDir = std::filesystem::path(EGSDK::Utils::Files::GetCurrentProcDirectory());
		return Normalize(runtimeDir / ".." / ".." / ".." / "source" / "data" / "EGameTools");
	}

	std::filesystem::path GetUserModFilesDir() {
		return Normalize(GetSourceDataDir() / "UserModFiles");
	}

	std::filesystem::path GetSettingsFilePath() {
		return Normalize(GetRuntimeDataDir() / "settings.toml");
	}

	std::filesystem::path GetLegacyConfigFilePath() {
		return Normalize(std::filesystem::path(EGSDK::Utils::Files::GetCurrentProcDirectory()) / "EGameTools.ini");
	}

	std::filesystem::path GetTeleportFilePath() {
		return Normalize(GetRuntimeDataDir() / "teleport_locations.toml");
	}

	std::filesystem::path GetMigrationReportPath() {
		return Normalize(GetRuntimeDataDir() / "migration-report.txt");
	}

	std::filesystem::path GetCrashDumpPath() {
		return Normalize(GetRuntimeDataDir() / "EGameTools-dump.dmp");
	}

	std::filesystem::path GetGameRootDir() {
		const auto sourceDataDir = GetSourceDataDir();
		return Normalize(sourceDataDir.parent_path().parent_path().parent_path().parent_path());
	}
}
