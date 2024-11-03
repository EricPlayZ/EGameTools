#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <string_view>

namespace Utils {
	namespace Memory {
		static const BYTE SigScanWildCard = 0xAA;
		static const std::string_view SigScanWildCardStr = "AA";

		extern const MODULEINFO GetModuleInfo(const char* szModule);
		extern const FARPROC GetProcAddr(const std::string_view& module, const std::string_view& funcName);

		extern const bool IsAddressValidMod(const DWORD64 ptr, const char* moduleName);

		extern std::string BytesToIDAPattern(BYTE* bytes, size_t size);
		extern std::string ConvertSigToScannerSig(const char* pattern, int* offsetToAddrInSig = nullptr);

		extern DWORD64 CalcTargetAddrOfRelInst(DWORD64 addrOfInst, size_t opSize);
		std::vector<DWORD64> GetXrefsTo(DWORD64 address, DWORD64 start, size_t size);

		// Templates
		template<typename ptrT> bool IsValidPtr(ptrT ptr) {
			__try {
				return !IsBadReadPtr(reinterpret_cast<LPVOID>(ptr), sizeof(LPVOID));
			} __except(EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
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
