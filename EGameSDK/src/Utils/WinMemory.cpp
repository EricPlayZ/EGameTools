#include <unordered_map>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::Utils {
	namespace Memory {
		HMODULE GetCallingDLLModule(void* callerAddress) {
			HMODULE callerModule{};
			if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)callerAddress, &callerModule))
				return callerModule;
			return nullptr;
		}
		MODULEINFO GetModuleInfo(const char* szModule) {
			if (!szModule)
				return MODULEINFO();

			static std::unordered_map<std::string_view, MODULEINFO> moduleInfoCache;
			auto it = moduleInfoCache.find(szModule);
			if (it != moduleInfoCache.end())
				return it->second;

			HMODULE hModule = GetModuleHandle(szModule);
			if (hModule == 0)
				return MODULEINFO();

			MODULEINFO moduleInfo{};
			GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO));
			moduleInfoCache[szModule] = moduleInfo;

			return moduleInfoCache[szModule];
		}
		FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName) {
			HMODULE moduleHandle = GetModuleHandle(module.data());
			if (!moduleHandle)
				return nullptr;

			return GetProcAddress(moduleHandle, funcName.data());
		}
	}
}