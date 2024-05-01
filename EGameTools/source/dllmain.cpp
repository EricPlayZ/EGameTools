#include <pch.h>

namespace Core {
    extern void DisableConsole();

    extern DWORD64 WINAPI MainThread(HMODULE hModule);
    extern void Cleanup();
}

namespace Engine {
    namespace Hooks {
        extern Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, UINT, UINT, DWORD64*, DWORD64(*)(DWORD64, DWORD, DWORD64, char*, int), INT16, DWORD64, UINT)> MountDataPaksHook;
        extern Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, DWORD, DWORD)> FsOpenHook;
        extern Utils::Hook::MHook<LPVOID, bool(*)(LPVOID, LPVOID)> FsCalcFileCrcHook;
        extern Utils::Hook::MHook<LPVOID, LPVOID(*)(LPVOID)> AuthenticateDataAddNewFileHook;
        extern Utils::Hook::MHook<LPVOID, void(*)(LPVOID)> AuthenticateDataAnalyzeHook;
        extern Utils::Hook::MHook<LPVOID, bool(*)(LPVOID)> FsCheckZipCrcHook;
    }
}

static HANDLE hMainThread{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD64 ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        MH_Initialize();
        Engine::Hooks::MountDataPaksHook.HookLoop();
        Engine::Hooks::AuthenticateDataAnalyzeHook.HookLoop();
        Engine::Hooks::AuthenticateDataAddNewFileHook.HookLoop();
        Engine::Hooks::FsCheckZipCrcHook.HookLoop();
        Engine::Hooks::FsCalcFileCrcHook.HookLoop();
        Engine::Hooks::FsOpenHook.HookLoop();

        DisableThreadLibraryCalls(hModule);
        hMainThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Core::MainThread, hModule, 0, nullptr);

        if (!hMainThread)
            return FALSE;
        break;
    }
    case DLL_PROCESS_DETACH:
        Core::Cleanup();
        Core::DisableConsole();

        if (hMainThread)
            CloseHandle(hMainThread);
        FreeLibraryAndExitThread(hModule, 0);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

