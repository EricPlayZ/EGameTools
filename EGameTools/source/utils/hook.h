#pragma once
#include <functional>
#include <set>
#include <atomic>
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

			virtual bool HookLoop() { return false; };

			const std::string_view name{};
			std::atomic<bool> isHooking = false;
			std::atomic<bool> isHooked = false;
		};
		template <typename GetTargetOffsetRetType>
		class ByteHook : HookBase {
		public:
			ByteHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), unsigned char* patchBytes, size_t bytesAmount, Option* optionRef = nullptr) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), patchBytes(patchBytes), bytesAmount(bytesAmount), optionRef(optionRef) {}

			bool HookLoop() override {
				if (isHooked || (optionRef && !optionRef->GetValue()))
					return true;
				if (isHooking)
					return true;

				isHooking = true;
				timeSpentHooking = Utils::Time::Timer(120000);

				while (true) {
					if (timeSpentHooking.DidTimePass()) {
						spdlog::error("Failed hooking \"{}\" after 120 seconds", name);
						isHooking = false;
						return false;
					}
					if (!pGetOffsetFunc || !pGetOffsetFunc())
						continue;

					DWORD originalProtection = 0;
					DWORD oldProtection = 0;

					VirtualProtect(pGetOffsetFunc(), bytesAmount, PAGE_EXECUTE_READWRITE, &originalProtection);
					if (!origBytes) {
						origBytes = new unsigned char[bytesAmount];
						memcpy_s(origBytes, bytesAmount, pGetOffsetFunc(), bytesAmount);
					}
					memcpy_s(pGetOffsetFunc(), bytesAmount, patchBytes, bytesAmount);
					VirtualProtect(pGetOffsetFunc(), bytesAmount, originalProtection, &oldProtection);
					isHooked = true;
					isHooking = false;
					return true;

					Sleep(10);
				}
			}

			void Enable() {
				if (isHooked)
					return;

				DWORD originalProtection = 0;
				DWORD oldProtection = 0;

				VirtualProtect(pGetOffsetFunc(), bytesAmount, PAGE_EXECUTE_READWRITE, &originalProtection);
				if (!origBytes) {
					origBytes = new unsigned char[bytesAmount];
					memcpy_s(origBytes, bytesAmount, pGetOffsetFunc(), bytesAmount);
				}
				memcpy_s(pGetOffsetFunc(), bytesAmount, patchBytes, bytesAmount);
				VirtualProtect(pGetOffsetFunc(), bytesAmount, originalProtection, &oldProtection);
				isHooked = true;
			}
			void Disable() {
				if (!isHooked)
					return;

				DWORD originalProtection = 0;
				DWORD oldProtection = 0;

				VirtualProtect(pGetOffsetFunc(), bytesAmount, PAGE_EXECUTE_READWRITE, &originalProtection);
				memcpy_s(pGetOffsetFunc(), bytesAmount, origBytes, bytesAmount);
				VirtualProtect(pGetOffsetFunc(), bytesAmount, originalProtection, &oldProtection);
				isHooked = false;
			}
			void Toggle() {
				if (!optionRef) {
					isHooked ? Disable() : Enable();
					return;
				}

				optionRef->GetValue() ? Enable() : Disable();
			}

			Option* optionRef = nullptr;
		private:
			GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
			unsigned char* origBytes = nullptr;
			unsigned char* patchBytes = nullptr;
			size_t bytesAmount = 0;

			Utils::Time::Timer timeSpentHooking{ 120000 };
		};
		template <typename GetTargetOffsetRetType, typename OrigType>
		class MHook : HookBase {
		public:
			MHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour) {}

			bool HookLoop() override {
				if (pOriginal)
					return true;
				if (isHooking)
					return true;

				isHooking = true;
				timeSpentHooking = Utils::Time::Timer(120000);

				while (true) {
					if (timeSpentHooking.DidTimePass()) {
						spdlog::error("Failed hooking function \"{}\" after 120 seconds", name);
						isHooking = false;
						return false;
					}
					if (!pGetOffsetFunc)
						continue;

					if (!pTarget)
						pTarget = reinterpret_cast<OrigType>(pGetOffsetFunc());
					else if (!pOriginal && MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(&pOriginal)) == MH_OK) {
						MH_EnableHook(pTarget);
						isHooked = true;
						isHooking = false;
						return true;
					}

					Sleep(10);
				}
			}

			OrigType pOriginal = nullptr;
			OrigType pTarget = nullptr;
		private:
			GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
			OrigType pDetour = nullptr;

			Utils::Time::Timer timeSpentHooking{ 120000 };
		};
		template <typename GetTargetOffsetRetType, typename OrigType>
		class VTHook : HookBase {
		public:
			VTHook(const std::string_view& name, GetTargetOffsetRetType(*pGetOffsetFunc)(), OrigType pDetour, DWORD offset) : HookBase(name), pGetOffsetFunc(pGetOffsetFunc), pDetour(pDetour), offset(offset) {}

			bool HookLoop() override {
				if (pOriginal)
					return true;
				if (isHooking)
					return true;

				isHooking = true;
				timeSpentHooking = Utils::Time::Timer(120000);

				while (true) {
					if (timeSpentHooking.DidTimePass()) {
						spdlog::error("Failed hooking function \"{}\" after 120 seconds", name);
						isHooking = false;
						return false;
					}
					if (!pGetOffsetFunc)
						continue;

					if (!pInstance)
						pInstance = pGetOffsetFunc();
					else if (!pOriginal) {
						HookVT(pInstance, pDetour, reinterpret_cast<LPVOID*>(&pOriginal), offset);
						if (pOriginal) {
							isHooked = true;
							isHooking = false;
							return true;
						}
					}

					Sleep(10);
				}
			}

			OrigType pOriginal = nullptr;
		private:
			GetTargetOffsetRetType(*pGetOffsetFunc)() = nullptr;
			LPVOID pInstance = nullptr;
			OrigType pDetour = nullptr;

			Utils::Time::Timer timeSpentHooking{ 120000 };

			DWORD offset = 0x0;
		};
	}
}