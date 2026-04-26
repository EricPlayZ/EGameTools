#pragma once
#include <filesystem>
#include <string>

namespace EGT::Config::TeleportStore {
	void Init();
	bool Save();
	bool ReloadFromDisk();
	bool HasInMemoryChanges();
	bool FileExists();
	const std::filesystem::path& GetTeleportFilePath();
	void ImportSerializedLocations(const std::string& serializedLocations);
}
