#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <string_view>

namespace Utils {
	namespace Memory {
		extern const MODULEINFO GetModuleInfo(const char* szModule);
		extern const FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName);

		extern const bool IsAddressValidMod(const DWORD64 ptr, const char* moduleName);

		// Templates
		template<typename ptrT> bool IsValidPtr(ptrT ptr) {
			return !IsBadReadPtr(reinterpret_cast<LPVOID>(ptr), sizeof(LPVOID));
		}
		template<typename ptrT = LPVOID> bool IsValidPtrMod(ptrT ptr, const char* moduleName, const bool checkForVT = true) {
			return IsValidPtr<ptrT>(ptr) && IsAddressValidMod(checkForVT ? *(PDWORD64)(ptr) : (DWORD64)(ptr), moduleName);
		}

		template <std::size_t Index, typename ReturnType = void, typename... Args> __forceinline ReturnType CallVT(LPVOID instance, Args... args) {
			using Fn = ReturnType(__thiscall*)(LPVOID, Args...);

			auto function = (*reinterpret_cast<Fn**>(instance))[Index];
			return function(instance, args...);
		}
		template <std::size_t Index, typename ReturnType = void, typename... Args> __forceinline ReturnType CallFromVT(LPVOID instance, LPVOID vtable, Args... args) {
			using Fn = ReturnType(__thiscall*)(LPVOID, Args...);

			auto function = reinterpret_cast<Fn*>(vtable)[Index];
			return function(instance, args...);
		}
	}
}
