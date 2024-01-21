#include <Windows.h>
#include <imgui.h>
#include <backends\imgui_impl_win32.h>
#include <Hotkey.h>
#include "..\..\menu\menu.h"
#include "..\..\sigscan\offsets.h"
#include "..\..\game_classes.h"
#include "..\..\kiero.h"
#include "win32_impl.h"

static WNDPROC oWndProc = NULL;
static bool toggledMenu = false;
static bool wasAnyKeyPressed = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT __stdcall hkWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	switch (uMsg) {
	case WM_KEYDOWN:
		if (ImGui::isAnyHotkeyBtnPressed || !ImGui::timeSinceHotkeyBtnPressed.GetTimePassed() || wasAnyKeyPressed)
			break;

		if (wParam == Menu::toggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::isOpen = !Menu::isOpen;
		}
		else if (wParam == Menu::Player::godModeToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Player::godModeEnabled.value = !Menu::Player::godModeEnabled.value;
		}
		else if (wParam == Menu::Player::freezePlayerToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Player::freezePlayerEnabled.value = !Menu::Player::freezePlayerEnabled.value;
		}
		else if (wParam == Menu::Camera::freeCamToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Camera::freeCamEnabled.value = !Menu::Camera::freeCamEnabled.value;
		}
		else if (wParam == Menu::Camera::teleportPlayerToCameraToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Camera::teleportPlayerToCameraEnabled = !Menu::Camera::teleportPlayerToCameraEnabled;
		}
		else if (wParam == Menu::Camera::thirdPersonCameraToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Camera::thirdPersonCameraEnabled.value = !Menu::Camera::thirdPersonCameraEnabled.value;
		}
		else if (wParam == Menu::Camera::tpUseTPPModelToggleKey.toInt()) {
			wasAnyKeyPressed = true;
			Menu::Camera::tpUseTPPModelEnabled.value = !Menu::Camera::tpUseTPPModelEnabled.value;
		}
		break;
	case WM_KEYUP:
		if (!wasAnyKeyPressed)
			break;

		if (wParam == Menu::toggleKey.toInt() ||
			wParam == Menu::Player::godModeToggleKey.toInt() ||
			wParam == Menu::Player::freezePlayerToggleKey.toInt() ||
			wParam == Menu::Camera::freeCamToggleKey.toInt() ||
			wParam == Menu::Camera::teleportPlayerToCameraToggleKey.toInt() ||
			wParam == Menu::Camera::thirdPersonCameraToggleKey.toInt() ||
			wParam == Menu::Camera::tpUseTPPModelToggleKey.toInt())
			wasAnyKeyPressed = false;
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