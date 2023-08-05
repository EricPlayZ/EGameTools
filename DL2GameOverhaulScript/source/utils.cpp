#include <Windows.h>
#include <shlobj.h>
#include "utils.h"

namespace Utils {
    bool str_replace(std::string& str, const std::string& from, const std::string& to) {
        const size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;

        str.replace(start_pos, from.length(), to);
        return true;
    }

    std::string_view GetDesktopDir() {
        char path[MAX_PATH + 1];
        if (!SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
            return {};

        return path;
    }
    WindowsVersion GetWindowsVersion() {
        OSVERSIONINFOEX info{};
        DWORDLONG dwlConditionMask = 0;

        ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        info.dwMajorVersion = 6;

        VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);

        if (VerifyVersionInfo(&info, VER_MAJORVERSION, dwlConditionMask)) {
            // The major version is 6.
            info.dwMinorVersion = 1;
            VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);

            if (VerifyVersionInfo(&info, VER_MINORVERSION, dwlConditionMask))
                return WindowsVersion::Windows7;

            info.dwMinorVersion = 2;
            VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);

            if (VerifyVersionInfo(&info, VER_MINORVERSION, dwlConditionMask))
                return WindowsVersion::Windows10; // Windows 8 and 8.1 also use 6.2

            info.dwMinorVersion = 3;
            VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);

            if (VerifyVersionInfo(&info, VER_MINORVERSION, dwlConditionMask))
                return WindowsVersion::Windows10;
        } else {
            // The major version is not 6. Check if it's greater than 6.
            info.dwMajorVersion = 7;
            VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);

            if (VerifyVersionInfo(&info, VER_MAJORVERSION, dwlConditionMask))
                return WindowsVersion::Windows10;
        }

        return WindowsVersion::Unknown;
    }
}