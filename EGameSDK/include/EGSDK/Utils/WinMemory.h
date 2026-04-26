#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <string_view>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Exports.h>

namespace EGSDK::Utils {
	namespace Memory {
		extern EGameSDK_API HMODULE GetCallingDLLModule(void* callerAddress);
		extern EGameSDK_API MODULEINFO GetModuleInfo(const char* szModule);
		extern EGameSDK_API FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName);

#pragma region Templates
		template <typename ReturnType, typename... Args>
		__forceinline ReturnType SafeCallFunction(const char* moduleName, const char* functionName, ReturnType ret, Args... args) {
			using FunctionType = ReturnType(__stdcall*)(Args...);
			FunctionType function = reinterpret_cast<FunctionType>(GetProcAddr(moduleName, functionName));
			if (!function)
				return ret;

			__try {
				return function(args...);
			} __except (SafeExecution::fail(GetExceptionCode(), GetExceptionInformation())) {
				return ret;
			}
		}
		template <typename... Args>
		__forceinline bool SafeCallFunctionVoid(const char* moduleName, const char* functionName, Args... args) {
			using FunctionType = void(__stdcall*)(Args...);
			FunctionType function = reinterpret_cast<FunctionType>(GetProcAddr(moduleName, functionName));
			if (!function)
				return false;

			__try {
				function(args...);
				return true;
			} __except (SafeExecution::fail(GetExceptionCode(), GetExceptionInformation())) {
				return false;
			}
		}
#pragma endregion
	}
}