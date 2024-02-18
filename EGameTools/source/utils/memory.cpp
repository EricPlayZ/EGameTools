#include <pch.h>

namespace Utils {
	namespace Memory {
		const MODULEINFO GetModuleInfo(const char* szModule) {
			const HMODULE hModule = GetModuleHandle(szModule);
			if (hModule == 0)
				return MODULEINFO();

			MODULEINFO moduleInfo{};
			GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));
			return moduleInfo;
		}
		const FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName) {
			const HMODULE moduleHandle = GetModuleHandleA(module.data());
			if (!moduleHandle)
				return nullptr;

			return GetProcAddress(moduleHandle, funcName.data());
		}

		const bool IsAddressValidMod(const DWORD64 ptr, const char* moduleName) {
			const MODULEINFO moduleInf = GetModuleInfo(moduleName);

			const DWORD64 moduleEntryPoint = reinterpret_cast<DWORD64>(moduleInf.EntryPoint);
			const DWORD64 moduleEndPoint = moduleEntryPoint + moduleInf.SizeOfImage;
			return ptr && (ptr <= moduleEndPoint && ptr >= moduleEntryPoint);
		}
	}
}