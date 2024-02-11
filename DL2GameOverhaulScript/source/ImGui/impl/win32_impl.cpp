#include <pch.h>
#include "..\core.h"
#include "..\game\Engine\CInput.h"
#include "..\menu\menu.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace impl {
	namespace win32 {
		static HWND gHwnd = nullptr;
		static WNDPROC oWndProc = nullptr;
		static HHOOK oMouseProc = nullptr;

		static LRESULT __stdcall hkWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
			switch (uMsg) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if (Menu::firstTimeRunning.GetValue() || ImGui::isAnyHotkeyBtnPressed || !ImGui::timeSinceHotkeyBtnPressed.DidTimePass() || KeyBindOption::wasAnyKeyPressed)
					break;

				for (auto& option : *KeyBindOption::GetInstances()) {
					if (option->GetChangesAreDisabled())
						continue;
					if (option->GetKeyBind() == VK_NONE)
						continue;

					if (wParam == option->GetKeyBind()) {
						KeyBindOption::wasAnyKeyPressed = true;
						option->Toggle();
					}
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if (!KeyBindOption::wasAnyKeyPressed)
					break;

				for (auto& option : *KeyBindOption::GetInstances()) {
					if (wParam == option->GetKeyBind())
						KeyBindOption::wasAnyKeyPressed = false;
				}
				break;
			}

			Engine::CInput* pCInput = Engine::CInput::Get();
			if (!pCInput)
				return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);

			ImGui::GetIO().MouseDrawCursor = Menu::firstTimeRunning.GetValue() || Menu::menuToggle.GetValue();
			ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

			if (Menu::firstTimeRunning.GetValue() || Menu::menuToggle.GetValue()) {
				pCInput->BlockGameInput();

				if (Menu::menuToggle.GetValue())
					Menu::menuToggle.SetPrevValue(true);
			} else if (Menu::firstTimeRunning.GetPrevValue() || Menu::menuToggle.GetPrevValue()) {
				if (Menu::firstTimeRunning.GetPrevValue())
					Menu::firstTimeRunning.SetPrevValue(false);
				else if (Menu::menuToggle.GetPrevValue())
					Menu::menuToggle.SetPrevValue(false);
				pCInput->UnlockGameInput();
			}

			return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);
		}

#ifndef LLMH_IMPL_DISABLE_DEBUG
		static LRESULT CALLBACK hkMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
			if (nCode != HC_ACTION)
				return CallNextHookEx(oMouseProc, nCode, wParam, lParam);
			if (GetForegroundWindow() != gHwnd)
				return CallNextHookEx(oMouseProc, nCode, wParam, lParam);

			switch (wParam) {
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
			{
				MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
				if (pMouseStruct == nullptr)
					break;

				if (GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData)) {
					if (Menu::firstTimeRunning.GetValue())
						break;
					for (auto& option : *KeyBindOption::GetInstances()) {
						if (option->GetChangesAreDisabled())
							continue;

						if ((option->GetKeyBind() == VK_MWHEELUP && GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) > 0) ||
							(option->GetKeyBind() == VK_MWHEELDOWN && GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) < 0))
							option->Toggle();
					}

					if (GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) > 0)
						KeyBindOption::scrolledMouseWheelUp = true;
					else if (GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData) < 0)
						KeyBindOption::scrolledMouseWheelDown = true;
				}
				break;
			}
			}

			return CallNextHookEx(oMouseProc, nCode, wParam, lParam);
		}
#endif

		void init(HWND hwnd) {
			gHwnd = hwnd;
			oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)hkWindowProc);
#ifndef LLMH_IMPL_DISABLE_DEBUG
			oMouseProc = SetWindowsHookEx(WH_MOUSE_LL, hkMouseProc, GetModuleHandle(nullptr), 0);

			MSG msg;
			while (oMouseProc && GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
#endif
		}
	}
}