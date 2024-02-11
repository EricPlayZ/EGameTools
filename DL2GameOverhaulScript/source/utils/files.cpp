#include <pch.h>

namespace Utils {
    namespace Files {
        const std::string_view GetDesktopDir() {
            char path[MAX_PATH + 1]{};
            if (!SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
                return {};

            return path;
        }
        const std::string_view GetDocumentsDir() {
            char path[MAX_PATH + 1]{};
            if (SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, path) != S_OK)
                return {};

            return path;
        }
        const std::filesystem::path GetCurrentProcDirectoryFS() {
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
            return std::filesystem::path(buffer).parent_path();
        }
        const std::string GetCurrentProcDirectory() {
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
            return std::filesystem::path(buffer).parent_path().string();
        }
        const bool FileExistsInDir(const char* fileName, const char* dir) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                const std::filesystem::path pathToFile = entry.path();
                if (!std::filesystem::is_regular_file(pathToFile))
                    continue;

                const std::filesystem::path pathToFilename = pathToFile.filename();
                if (!pathToFilename.string().contains(fileName))
                    continue;

                return true;
            }
            return false;
        }
    }
}