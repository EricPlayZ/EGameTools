#include "hook.h"
#include "sigscan\offsets.h"
#include <cstdint>
#include <iostream>

namespace Hook {
	// VTable hooking
	static int VTHookUnprotect(LPVOID region) {
		MEMORY_BASIC_INFORMATION mbi{};
		VirtualQuery(static_cast<LPCVOID>(region), &mbi, sizeof(mbi));
		VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);
		return mbi.Protect;
	}
	static void VTHookProtect(LPVOID region, int protection) {
		MEMORY_BASIC_INFORMATION mbi{};
		VirtualQuery(static_cast<LPCVOID>(region), &mbi, sizeof(mbi));
		VirtualProtect(mbi.BaseAddress, mbi.RegionSize, protection, &mbi.Protect);
	}

	void VTHook(LPVOID instance, LPVOID pDetour, LPVOID* ppOriginal, const DWORD offset) {
		const DWORD64 vtable = *static_cast<DWORD64*>(instance);
		DWORD64** entry = reinterpret_cast<DWORD64**>(vtable + offset);
		*ppOriginal = *entry;

		const int original_protection = VTHookUnprotect(entry);
		*entry = static_cast<DWORD64*>(pDetour);
		VTHookProtect(entry, original_protection);
	}

	// Trampoline hooking
	DETOUR_INFO MidFuncHook(DWORD64* pTarget, DWORD64* pHookedFunc, const size_t nops, const size_t bytesToSkip) {
		DWORD dwOldProtect, dwBkup{};

		const size_t finalHookSize = 13 + nops;

		DETOUR_INFO detour{ pTarget, pHookedFunc };
		detour.oBytes.resize(finalHookSize);
		detour.patchedBytes.resize(finalHookSize);

		VirtualProtect(pTarget, finalHookSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		BYTE* temp_target = reinterpret_cast<BYTE*>(pTarget);
		std::copy(temp_target, temp_target + finalHookSize, detour.oBytes.begin());

		// push rax
		*temp_target = 0x50;
		temp_target++;

		// mov rax, hookedFuncAddr
		const DWORD64 hookedFuncAddr = reinterpret_cast<DWORD64>(pHookedFunc);
		*temp_target = 0x48;
		*(temp_target + 1) = 0xB8;
		*reinterpret_cast<DWORD64*>(temp_target + 2) = hookedFuncAddr + bytesToSkip;
		temp_target += 2 + sizeof(DWORD64);

		// jmp rax
		*temp_target = 0xFF;
		*(temp_target + 1) = 0xE0;
		temp_target += 2;

		// nop
		if (nops != 0) {
			for (int i = 0; i < nops; i++) {
				*temp_target = 0x90;
				temp_target++;
			}
		}

		detour.jmpBack = reinterpret_cast<DWORD64*>(temp_target);
		temp_target = reinterpret_cast<BYTE*>(pTarget);
		std::copy(temp_target, temp_target + finalHookSize, detour.patchedBytes.begin());

		VirtualProtect(pTarget, finalHookSize, dwOldProtect, &dwBkup);

		return detour;
	}

	// Breakpoint hooking
	static bool s_handlerCreated = false;
	static std::vector<BreakpointHook*> s_hookList;

	BreakpointHook::BreakpointHook(DWORD64 addr, std::function<void(PEXCEPTION_POINTERS)> handler) {
		m_addr = addr;
		m_handler = handler;
		m_originalBytes = *(BYTE*)m_addr;

		if (!s_handlerCreated)
			AddVectoredExceptionHandler(true, (PVECTORED_EXCEPTION_HANDLER)OnException);

		s_hookList.push_back(this);

		Enable();
	}
	void BreakpointHook::Enable() {
		DWORD oldProtection = 0;

		VirtualProtect((LPVOID)m_addr, 1, PAGE_EXECUTE_READWRITE, &m_originalProtection);
		*(BYTE*)m_addr = 0xCC;
		VirtualProtect((LPVOID)m_addr, 1, m_originalProtection, &oldProtection);
	}
	void BreakpointHook::Disable() {
		DWORD oldProtection = 0;

		VirtualProtect((LPVOID)m_addr, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
		*(BYTE*)m_addr = m_originalBytes;
		VirtualProtect((LPVOID)m_addr, 1, m_originalProtection, &oldProtection);
	}
	BreakpointHook::~BreakpointHook() {
		Disable();

		auto it = std::find(s_hookList.begin(), s_hookList.end(), this);

		if (it != s_hookList.end())
			s_hookList.erase(it);
	}

	static DWORD64 lastBpAddress = NULL;
	long WINAPI BreakpointHook::OnException(PEXCEPTION_POINTERS info) {
		for (auto it = s_hookList.begin(); it != s_hookList.end(); it++) {
			BreakpointHook* bp = *it;

			if (bp->m_addr == reinterpret_cast<DWORD64>(info->ExceptionRecord->ExceptionAddress)) {
				bp->Disable();

				lastBpAddress = bp->m_addr;
				info->ContextRecord->EFlags |= 0x100; // Set EFLAGS to single step

				return EXCEPTION_CONTINUE_EXECUTION;
			} else if (info->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP && bp->m_addr == lastBpAddress) {
				bp->Enable();
				info->ContextRecord->EFlags &= ~0x00000100; // Remove TRACE from EFLAGS

				bp->m_handler(info);

				return EXCEPTION_CONTINUE_EXECUTION;
			}
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}
}