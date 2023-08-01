#pragma once
#include <Windows.h>
#include <functional>

namespace Hook {
	struct DETOUR_INFO {
		LPVOID target;
		LPVOID hookedFunc;
		PDWORD64 jmpBack;
		std::vector<BYTE> oBytes;
		std::vector<BYTE> patchedBytes;

		DETOUR_INFO() = default;
		DETOUR_INFO(LPVOID pTarget, LPVOID pHookedFunc) {
			target = pTarget;
			hookedFunc = pHookedFunc;
			jmpBack = nullptr;
		}
	};

	extern void VTHook(LPVOID instance, LPVOID pDetour, LPVOID* ppOriginal, const DWORD offset);

	extern DETOUR_INFO MidFuncHook(LPVOID pTarget, LPVOID hookedFunc, const size_t nops = 0, const size_t bytesToSkip = 0);

	class BreakpointHook {
	private:
		PDWORD64 m_addr;
		BYTE m_originalBytes;
		std::function<void(PEXCEPTION_POINTERS)> m_handler;
		DWORD m_originalProtection;

		static long WINAPI OnException(PEXCEPTION_POINTERS info);
	public:
		BreakpointHook(PDWORD64 addr, std::function<void(PEXCEPTION_POINTERS)> handler);
		void Enable();
		void Disable();
		~BreakpointHook();
	};
}