#pragma once
#include <filesystem>
#include <string>

namespace Utils {
	namespace Files {
		extern const std::string GetDesktopDir();
		extern const std::string GetDocumentsDir();
		extern const std::string GetLocalAppDataDir();
		extern const std::filesystem::path GetCurrentProcDirectoryFS();
		extern const std::string GetCurrentProcDirectory();
		extern const bool FileExistsInDir(const char* fileName, const char* dir);
	}
}