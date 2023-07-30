#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <string_view>

namespace Memory {
	extern const MODULEINFO GetModuleInfo(const std::string_view szModule);

	extern const bool IsAddressValidMod(const DWORD64 ptr, const std::string_view moduleName);

	template<typename ptrT>
	const bool IsValidPtr(ptrT ptr) {
		return !IsBadReadPtr(reinterpret_cast<LPVOID>(ptr), sizeof(LPVOID));
	}

	template<typename ptrT = LPVOID>
	const bool IsValidPtrMod(ptrT ptr, const std::string_view moduleName, const bool checkForVT = true) {
		return IsValidPtr<ptrT>(ptr) && IsAddressValidMod(checkForVT ? *(DWORD64*)(ptr) : (DWORD64)(ptr), moduleName);
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