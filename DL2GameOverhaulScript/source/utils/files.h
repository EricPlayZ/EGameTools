#pragma once
#include <filesystem>
#include <string_view>

namespace Utils {
	namespace Files {
		extern const std::string_view GetDesktopDir();
		extern const std::string_view GetDocumentsDir();
		extern const std::filesystem::path GetCurrentProcDirectoryFS();
		extern const std::string GetCurrentProcDirectory();
		extern const bool FileExistsInDir(const char* fileName, const char* dir);
	}
}