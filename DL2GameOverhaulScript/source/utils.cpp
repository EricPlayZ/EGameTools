#include <Windows.h>
#include <filesystem>
#include <shlobj.h>
#include "utils.h"

namespace Utils {
    Timer::Timer(long timeMs) : timeToPass(std::chrono::milliseconds(timeMs)), timePassed(false) {
        start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
    }

    long Timer::GetTimePassed() {
        if (timePassed)
            return -1;

        auto end = clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        long timePassedMs = duration.count();

        if (timePassedMs < 0) {
            start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
            return -1;
        }
        return timePassedMs;
    }
    bool Timer::DidTimePass() {
        if (timePassed)
            return timePassed;

        auto end = clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        long timePassedMs = duration.count();

        if (timePassedMs < 0) {
            start = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
            return false;
        }

        if (duration >= timeToPass)
            timePassed = true;
        return timePassed;
    }

    bool are_same(float a, float b) {
        return abs(a - b) < 0.0001f;
    }

    bool str_replace(std::string& str, const std::string& from, const std::string& to) {
        const size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;

        str.replace(start_pos, from.length(), to);
        return true;
    }

    FARPROC GetProcAddr(std::string_view module, std::string_view funcName) {
        HMODULE moduleHandle = GetModuleHandleA(module.data());
        if (!moduleHandle)
            return nullptr;

        return GetProcAddress(moduleHandle, funcName.data());
    }

    std::string_view GetDesktopDir() {
        char path[MAX_PATH + 1]{};
        if (!SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
            return {};

        return path;
    }
    std::string_view GetDocumentsDir() {
        char path[MAX_PATH + 1]{};
        if (SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, path) != S_OK)
            return {};

        return path;
    }
    std::string GetCurrentProcDirectory() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
        return std::filesystem::path(buffer).parent_path().string();
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