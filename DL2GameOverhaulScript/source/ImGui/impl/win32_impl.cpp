#include <Windows.h>
#include <imgui.h>
#include <backends\imgui_impl_win32.h>
#include "..\..\menu\menu.h"
#include "..\..\sigscan\offsets.h"
#include "..\..\kiero.h"
#include "..\..\game_classes.h"
#include "win32_impl.h"

static WNDPROC oWndProc = NULL;
static bool toggledMenu = false;
static bool menuMsgSent = false;
static bool wasMenuKeyPressed = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall hkWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	switch (uMsg) {
	case WM_KEYDOWN:
		if (wParam == VK_F5 && !wasMenuKeyPressed) {
			wasMenuKeyPressed = true;
			Menu::isOpen = !Menu::isOpen;
		}
		break;
	case WM_KEYUP:
		if (wParam == VK_F5 && wasMenuKeyPressed)
			wasMenuKeyPressed = false;
		break;
	}

	Engine::CInput* pCInput = Engine::CInput::Get();
	if (!pCInput)
		return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);

	ImGui::GetIO().MouseDrawCursor = Menu::isOpen;
	ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

	if (Menu::isOpen) {
		if (!toggledMenu)
			pCInput->BlockGameInput();

		toggledMenu = true;
		return true;
	} else if (toggledMenu) {
		toggledMenu = false;
		pCInput->UnlockGameInput();
	}

	return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);
}

void impl::win32::init(HWND hwnd) {
	oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)hkWindowProc);
}