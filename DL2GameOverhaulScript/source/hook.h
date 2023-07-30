#pragma once
#include <Windows.h>
#include <functional>

namespace Hook {
	struct DETOUR_INFO {
		DWORD64* target;
		DWORD64* hookedFunc;
		DWORD64* jmpBack;
		std::vector<BYTE> oBytes;
		std::vector<BYTE> patchedBytes;

		DETOUR_INFO() = default;
		DETOUR_INFO(DWORD64* pTarget, DWORD64* pHookedFunc) {
			target = pTarget;
			hookedFunc = pHookedFunc;
			jmpBack = nullptr;
		}
	};

	extern void VTHook(LPVOID instance, LPVOID pDetour, LPVOID* ppOriginal, const DWORD offset);

	extern const DETOUR_INFO MidFuncHook(DWORD64* pTarget, DWORD64* hookedFunc, const size_t nops = 0, const size_t bytesToSkip = 0);

	class BreakpointHook {
	private:
		DWORD64 m_addr;
		BYTE m_originalBytes;
		std::function<void(PEXCEPTION_POINTERS)> m_handler;
		DWORD m_originalProtection;

		static long WINAPI OnException(PEXCEPTION_POINTERS info);
	public:
		BreakpointHook(DWORD64 addr, std::function<void(PEXCEPTION_POINTERS)> handler);
		void Enable();
		void Disable();
		~BreakpointHook();
	};
}