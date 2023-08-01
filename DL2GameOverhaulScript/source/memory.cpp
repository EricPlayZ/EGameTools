#include <Windows.h>
#include <Psapi.h>

namespace Memory {
	MODULEINFO GetModuleInfo(const char* szModule) {
		const HMODULE hModule = GetModuleHandle(szModule);
		if (hModule == 0)
			return MODULEINFO();

		MODULEINFO moduleInfo{};
		GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));
		return moduleInfo;
	}

	bool IsAddressValidMod(const DWORD64 ptr, const char* moduleName) {
		MODULEINFO moduleInf = GetModuleInfo(moduleName);

		const DWORD64 moduleEntryPoint = reinterpret_cast<DWORD64>(moduleInf.EntryPoint);
		const DWORD64 moduleEndPoint = moduleEntryPoint + moduleInf.SizeOfImage;
		return ptr && (ptr <= moduleEndPoint && ptr >= moduleEntryPoint);
	}
}
