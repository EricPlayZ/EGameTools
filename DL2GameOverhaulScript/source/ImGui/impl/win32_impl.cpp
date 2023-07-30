#include <Windows.h>

#include "../../kiero.h"
#include "../imgui.h"
#include "../backends/imgui_impl_win32.h"

#include "win32_impl.h"

#include "../../menu/menu.h"
#include "../../sigscan/offsets.h"

static WNDPROC oWndProc = NULL;
static bool toggledMenu = false;
static bool wasMenuKeyPressed = false;

static void(*DoGameWindowStuffOnGainFocus)(LPVOID gui__CActionNode) = nullptr;
static void(*DoGameWindowStuffOnLostFocus)(LPVOID gui__CActionNode) = nullptr;
static LPVOID* g_gui__CActionNode = nullptr;

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

	ImGui::GetIO().MouseDrawCursor = Menu::isOpen;
	ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);

	if (!DoGameWindowStuffOnGainFocus) {
		DoGameWindowStuffOnGainFocus = (decltype(DoGameWindowStuffOnGainFocus))Offsets::Get_DoGameWindowStuffOnGainFocusOffset();
		return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);
	}
	if (!DoGameWindowStuffOnLostFocus) {
		DoGameWindowStuffOnLostFocus = (decltype(DoGameWindowStuffOnLostFocus))Offsets::Get_DoGameWindowStuffOnLostFocusOffset();
		return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);

	}
	if (!g_gui__CActionNode) {
		g_gui__CActionNode = Offsets::Get_g_gui__CActionNodeOffset();
		return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);
	}

	if (Menu::isOpen) {
		// Gain focus to ImGui
		if (!toggledMenu && DoGameWindowStuffOnLostFocus)
			DoGameWindowStuffOnLostFocus(g_gui__CActionNode);

		toggledMenu = true;
		return true;
	} else if (toggledMenu) {
		toggledMenu = false;

		// Regain focus to game
		DoGameWindowStuffOnGainFocus(g_gui__CActionNode);
	}

	return CallWindowProc(oWndProc, hwnd, uMsg, wParam, lParam);
}

void impl::win32::init(HWND hwnd) {
	oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)hkWindowProc);
}