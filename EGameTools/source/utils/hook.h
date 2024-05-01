#pragma once
#include <functional>
#include <set>
#include "..\MinHook\MinHook.h"

namespace Utils {
	namespace Hook {
		extern void HookVT(LPVOID instance, LPVOID pDetour, LPVOID* ppOriginal, const DWORD offset);

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
			HookBase(const std::string_view& name);
			~HookBase();
			static std::set<HookBase*>* GetInstances();

			virtual void HookLoop() {};

			const std::string_view name;
		};
		template <typename GetTargetOffsetRetType, typename OrigType>
		class MHook : HookBase {
		public:
			MHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour) {}

			void HookLoop() override {
				if (pOriginal)
					return;
				timeSpentHooking = Utils::Time::Timer(60000);

				while (true) {
					if (timeSpentHooking.DidTimePass()) {
						spdlog::error("Failed hooking function \"{}\" after 60 seconds", name);
						break;
					}
					if (!pGetOffsetFunc)
						continue;

					if (!pTarget)
						pTarget = reinterpret_cast<OrigType>(pGetOffsetFunc());
					else if (!pOriginal && MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(&pOriginal)) == MH_OK) {
						MH_EnableHook(pTarget);
						break;
					}
				}
			}

			OrigType pOriginal = nullptr;
			OrigType pTarget = nullptr;
		private:
			GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
			OrigType pDetour = nullptr;

			Utils::Time::Timer timeSpentHooking{ 60000 };
		};
		template <typename GetTargetOffsetRetType, typename OrigType>
		class VTHook : HookBase {
		public:
			VTHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour, DWORD offset) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour), offset(offset) {}

			void HookLoop() override {
				if (pOriginal)
					return;
				timeSpentHooking = Utils::Time::Timer(60000);

				while (true) {
					if (timeSpentHooking.DidTimePass()) {
						spdlog::error("Failed hooking function \"{}\" after 60 seconds", name);
						break;
					}
					if (!pGetOffsetFunc)
						continue;

					if (!pInstance)
						pInstance = pGetOffsetFunc();
					else if (!pOriginal) {
						HookVT(pInstance, pDetour, reinterpret_cast<LPVOID*>(&pOriginal), offset);
						break;
					}
				}
			}

			OrigType pOriginal = nullptr;
		private:
			GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
			LPVOID pInstance = nullptr;
			OrigType pDetour = nullptr;

			Utils::Time::Timer timeSpentHooking{ 60000 };

			DWORD offset = 0x0;
		};
	}
}