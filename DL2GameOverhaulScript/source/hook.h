#pragma once
#include <Windows.h>
#include <functional>
#include <set>
#include "MinHook\include\MinHook.h"
#include "print.h"

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

	extern void HookVT(LPVOID instance, LPVOID pDetour, LPVOID* ppOriginal, const DWORD offset);

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

	class HookBase {
	public:
		HookBase(const std::string_view& name) : name(name) { GetInstances()->insert(this); }
		~HookBase() { GetInstances()->erase(this); }
		static std::set<HookBase*>* GetInstances() { static std::set<HookBase*> instances{}; return &instances; };

		virtual void HookLoop() {};

		const std::string_view name;
	};
	template <typename GetTargetOffsetRetType, typename OrigType>
	class MHook : HookBase {
	public:
		MHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour) {}

		void HookLoop() override {
			while (true) {
				if (!pGetOffsetFunc)
					continue;

				if (!pTarget)
					pTarget = reinterpret_cast<OrigType>(pGetOffsetFunc());
				else if (!pOriginal && MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(&pOriginal)) == MH_OK) {
					MH_EnableHook(pTarget);
					break;
				}

				Sleep(250);
			}

			PrintSuccess("Hooked %s!", name.data());
		}

		OrigType pOriginal = nullptr;
		OrigType pTarget = nullptr;
	private:
		GetTargetOffsetRetType(*pGetOffsetFunc)()  = nullptr;
		OrigType pDetour = nullptr;
	};
	template <typename GetTargetOffsetRetType, typename OrigType>
	class VTHook : HookBase {
	public:
		VTHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour, DWORD offset) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour), offset(offset) {}

		void HookLoop() override {
			while (true) {
				if (!pGetOffsetFunc)
					continue;

				if (!pInstance)
					pInstance = pGetOffsetFunc();
				else if (!pOriginal) {
					Hook::HookVT(pInstance, pDetour, reinterpret_cast<LPVOID*>(&pOriginal), offset);
					break;
				}

				Sleep(250);
			}

			PrintSuccess("Hooked %s!", name.data());
		}

		OrigType pOriginal = nullptr;
	private:
		GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
		LPVOID pInstance = nullptr;
		OrigType pDetour = nullptr;

		DWORD offset = 0x0;
	};
}