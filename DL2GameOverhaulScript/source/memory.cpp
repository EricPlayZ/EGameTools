#include <Windows.h>
#include <Psapi.h>
#include <string_view>

namespace Memory {
	const MODULEINFO GetModuleInfo(const std::string_view szModule) {
		const HMODULE hModule = GetModuleHandle(szModule.data());
		if (hModule == 0)
			return MODULEINFO();

		MODULEINFO moduleInfo{};
		GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));
		return moduleInfo;
	}

	const bool IsAddressValidMod(const DWORD64 ptr, const std::string_view moduleName) {
		const MODULEINFO moduleInf = GetModuleInfo(moduleName);

		const DWORD64 moduleEntryPoint = reinterpret_cast<DWORD64>(moduleInf.EntryPoint);
		const DWORD64 moduleEndPoint = moduleEntryPoint + moduleInf.SizeOfImage;
		return ptr != NULL && (ptr <= moduleEndPoint && ptr >= moduleEntryPoint);
	}
}
