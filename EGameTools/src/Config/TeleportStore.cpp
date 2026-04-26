#include <filesystem>
#include <spdlog\spdlog.h>
#include <EGT\Config\ConfigPaths.h>
#include <EGT\Config\TomlDocument.h>
#include <EGT\Config\TeleportStore.h>
#include <EGT\Menu\Teleport.h>

namespace EGT::Config::TeleportStore {
	constexpr const char* sectionMeta = "meta";
	constexpr const char* sectionTeleport = "teleport";
	constexpr const char* schemaVersionKey = "schemaVersion";
	constexpr const char* locationsKey = "savedLocations";
	constexpr int schemaVersion = 1;
	const auto teleportFilePath = Paths::GetTeleportFilePath();
	std::string lastSerializedLocations{};

	void ApplySerializedLocations(const std::string& serializedLocations) {
		Menu::Teleport::savedTeleportLocations = Menu::Teleport::ParseTeleportLocations(serializedLocations);
		Menu::Teleport::UpdateTeleportLocationVisualNames();
		lastSerializedLocations = Menu::Teleport::ConvertTeleportLocationsToStr(Menu::Teleport::savedTeleportLocations);
	}

	bool FileExists() {
		return std::filesystem::exists(teleportFilePath);
	}

	const std::filesystem::path& GetTeleportFilePath() {
		return teleportFilePath;
	}

	void ImportSerializedLocations(const std::string& serializedLocations) {
		if (serializedLocations.empty())
			return;
		ApplySerializedLocations(serializedLocations);
	}

	bool ReloadFromDisk() {
		try {
			TomlDocument reader{};
			std::string errorMessage{};
			if (!reader.Load(teleportFilePath, &errorMessage)) {
				SPDLOG_ERROR("Failed loading teleport locations from {}: {}", teleportFilePath.string(), errorMessage);
				return false;
			}

			const std::string serializedLocations = reader.GetString(sectionTeleport, locationsKey, Menu::Teleport::savedTeleportLocationsStr);
			ApplySerializedLocations(serializedLocations);
			SPDLOG_INFO("Loaded teleport locations from {}", teleportFilePath.string());
			return true;
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Failed loading teleport locations from {}: {}", teleportFilePath.string(), e.what());
			return false;
		}
	}

	bool Save() {
		try {
			std::filesystem::create_directories(teleportFilePath.parent_path());

			TomlDocument writerState{};
			writerState.SetInt(sectionMeta, schemaVersionKey, schemaVersion);
			const std::string serializedLocations = Menu::Teleport::ConvertTeleportLocationsToStr(Menu::Teleport::savedTeleportLocations);
			writerState.SetString(sectionTeleport, locationsKey, serializedLocations);
			std::string errorMessage{};
			if (!writerState.Save(teleportFilePath, &errorMessage)) {
				SPDLOG_ERROR("Failed saving teleport locations to {}: {}", teleportFilePath.string(), errorMessage);
				return false;
			}
			lastSerializedLocations = serializedLocations;
			SPDLOG_INFO("Saved teleport locations to {}", teleportFilePath.string());
			return true;
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Failed saving teleport locations to {}: {}", teleportFilePath.string(), e.what());
			return false;
		}
	}

	bool HasInMemoryChanges() {
		const std::string currentSerializedLocations = Menu::Teleport::ConvertTeleportLocationsToStr(Menu::Teleport::savedTeleportLocations);
		return currentSerializedLocations != lastSerializedLocations;
	}

	void Init() {
		if (FileExists()) {
			if (ReloadFromDisk())
				return;
		}

		ApplySerializedLocations(Menu::Teleport::savedTeleportLocationsStr);
		Save();
	}
}
