#pragma once
#include <ShlObj.h>
#include <imgui.h>
#include <ranges>
#include <set>
#include "..\utils\values.h"


#ifndef VK_NONE

#ifdef _DEBUG
#define LLMH_IMPL_DISABLE_DEBUG     // this is for disabling low-level mouse hook in case ure trying to debug and u dont want ur pc to die lol
#endif

#define VK_NONE -1
#define VK_MWHEELDOWN 0x100
#define VK_MWHEELUP 0x101
#endif

constexpr auto MOD_VERSION_STR = "v1.1.0";
constexpr auto MOD_VERSION = 10100;
constexpr auto GAME_VER_COMPAT_STR = ">= v1.14.0";
constexpr auto GAME_VER_COMPAT = 11400;

struct Key {
    constexpr Key(std::string_view name, int code, ImGuiKey imGuiCode) : name(name), code(code), imGuiCode(imGuiCode) {}

    std::string_view name;
    int code;
    ImGuiKey imGuiCode;
};

class Option {
public:
    Option() { GetInstances()->insert(this); };
    ~Option() { GetInstances()->erase(this); }
    static std::set<Option*>* GetInstances() { static std::set<Option*> instances{}; return &instances; };

    bool value = false;

    void SetChangesAreDisabled(bool newValue) { changesAreDisabled = newValue; }
    bool GetChangesAreDisabled() const { return changesAreDisabled; }
    void Toggle() { previousValue = value; value = !value; }
    void Set(bool newValue) { previousValue = value; value = newValue; }
    void SetBothValues(bool newValue) { previousValue = newValue; value = newValue; }
    void SetValue(bool newValue) { value = newValue; }
    void SetPrevValue(bool newValue) { previousValue = newValue; }
    constexpr bool GetValue() const { return value; }
    constexpr bool GetPrevValue() const { return previousValue; }
    constexpr bool HasChanged() const { return previousValue != value; }
    constexpr bool HasChangedTo(bool toValue) const { return previousValue != value && value == toValue; }
private:
    bool changesAreDisabled = false;
	bool previousValue = false;
};
class KeyBindOption : public Option {
public:
    static bool wasAnyKeyPressed;
    static bool scrolledMouseWheelUp;
    static bool scrolledMouseWheelDown;

    KeyBindOption(int keyCode) : keyCode(keyCode) { GetInstances()->insert(this); };
    ~KeyBindOption() { GetInstances()->erase(this); }
    static std::set<KeyBindOption*>* GetInstances() { static std::set<KeyBindOption*> instances{}; return &instances; };

    const char* ToString() {
        if (const auto it = std::ranges::find(keyMap, keyCode, &Key::code); it != keyMap.end())
            return it->name.data();
        return "NONE";
    }
    constexpr int GetKeyBind() const { return keyCode; }
    void ChangeKeyBind(int newKeyBind) { keyCode = newKeyBind; }

    bool SetToPressedKey() {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ChangeKeyBind(VK_NONE);
            return true;
        } else if (!Utils::Values::are_samef(ImGui::GetIO().MouseWheel, 0.0f)) {
            ChangeKeyBind(ImGui::GetIO().MouseWheel < 0.0f ? VK_MWHEELDOWN : VK_MWHEELUP);
            return true;
        } else if (GetKeyState(VK_LSHIFT) & 0x8000) {
            ChangeKeyBind(VK_LSHIFT);
            return true;
        } else if (GetKeyState(VK_LMENU) & 0x8000) {
            ChangeKeyBind(VK_LMENU);
            return true;
        } else if (GetKeyState(VK_LCONTROL) & 0x8000) {
            ChangeKeyBind(VK_LCONTROL);
            return true;
        } else if (GetKeyState(VK_XBUTTON1) & 0x8000) {
            ChangeKeyBind(VK_XBUTTON1);
            return true;
        } else if (GetKeyState(VK_XBUTTON2) & 0x8000) {
            ChangeKeyBind(VK_XBUTTON2);
            return true;
        }

        for (int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().MouseDown); ++i) {
            if (ImGui::IsMouseClicked(i)) {
                ChangeKeyBind(i + 1);
                return true;
            }
        }

        for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i) {
            if (!ImGui::IsKeyPressed(static_cast<ImGuiKey>(i)))
                continue;

            if (const auto it = std::ranges::find(keyMap, i, &Key::imGuiCode); it != keyMap.end()) {
                ChangeKeyBind(it->code);
                // Treat AltGr as RALT
                if (GetKeyBind() == VK_LCONTROL && ImGui::IsKeyPressed(static_cast<ImGuiKey>(VK_RMENU)))
                    ChangeKeyBind(VK_RMENU);
                return true;
            }
        }
        return false;
    }
private:
    int keyCode = 0;
    static constexpr auto keyMap = std::to_array<Key>({
        { "'", VK_OEM_7, ImGuiKey_Apostrophe },
        { ",", VK_OEM_COMMA, ImGuiKey_Comma },
        { "-", VK_OEM_MINUS, ImGuiKey_Minus },
        { ".", VK_OEM_PERIOD, ImGuiKey_Period },
        { "/", VK_OEM_2, ImGuiKey_Slash },
        { "0", '0', ImGuiKey_0 },
        { "1", '1', ImGuiKey_1 },
        { "2", '2', ImGuiKey_2 },
        { "3", '3', ImGuiKey_3 },
        { "4", '4', ImGuiKey_4 },
        { "5", '5', ImGuiKey_5 },
        { "6", '6', ImGuiKey_6 },
        { "7", '7', ImGuiKey_7 },
        { "8", '8', ImGuiKey_8 },
        { "9", '9', ImGuiKey_9 },
        { ";", VK_OEM_1, ImGuiKey_Semicolon },
        { "=", VK_OEM_PLUS, ImGuiKey_Equal },
        { "A", 'A', ImGuiKey_A },
        { "ADD", VK_ADD, ImGuiKey_KeypadAdd },
        { "B", 'B', ImGuiKey_B },
        { "BACKSPACE", VK_BACK, ImGuiKey_Backspace },
        { "C", 'C', ImGuiKey_C },
        { "CAPSLOCK", VK_CAPITAL, ImGuiKey_CapsLock },
        { "D", 'D', ImGuiKey_D },
        { "DECIMAL", VK_DECIMAL, ImGuiKey_KeypadDecimal },
        { "DELETE", VK_DELETE, ImGuiKey_Delete },
        { "DIVIDE", VK_DIVIDE, ImGuiKey_KeypadDivide },
        { "DOWN", VK_DOWN, ImGuiKey_DownArrow },
        { "E", 'E', ImGuiKey_E },
        { "END", VK_END, ImGuiKey_End },
        { "ENTER", VK_RETURN, ImGuiKey_Enter },
        { "F", 'F', ImGuiKey_F },
        { "F1", VK_F1, ImGuiKey_F1 },
        { "F10", VK_F10, ImGuiKey_F10 },
        { "F11", VK_F11, ImGuiKey_F11 },
        { "F12", VK_F12, ImGuiKey_F12 },
        { "F2", VK_F2, ImGuiKey_F2 },
        { "F3", VK_F3, ImGuiKey_F3 },
        { "F4", VK_F4, ImGuiKey_F4 },
        { "F5", VK_F5, ImGuiKey_F5 },
        { "F6", VK_F6, ImGuiKey_F6 },
        { "F7", VK_F7, ImGuiKey_F7 },
        { "F8", VK_F8, ImGuiKey_F8 },
        { "F9", VK_F9, ImGuiKey_F9 },
        { "G", 'G', ImGuiKey_G },
        { "H", 'H', ImGuiKey_H },
        { "HOME", VK_HOME, ImGuiKey_Home },
        { "I", 'I', ImGuiKey_I },
        { "INSERT", VK_INSERT, ImGuiKey_Insert },
        { "J", 'J', ImGuiKey_J },
        { "K", 'K', ImGuiKey_K },
        { "L", 'L', ImGuiKey_L },
        { "LALT", VK_LMENU, ImGuiKey_LeftAlt },
        { "LCTRL", VK_LCONTROL, ImGuiKey_LeftCtrl },
        { "LEFT", VK_LEFT, ImGuiKey_LeftArrow },
        { "LSHIFT", VK_LSHIFT, ImGuiKey_LeftShift },
        { "M", 'M', ImGuiKey_M },
        { "MOUSE1", VK_LBUTTON, ImGuiKey_MouseLeft },
        { "MOUSE2", VK_RBUTTON, ImGuiKey_MouseRight },
        { "MOUSE3", VK_MBUTTON, ImGuiKey_MouseMiddle },
        { "MOUSE4", VK_XBUTTON1, ImGuiKey_MouseX1 },
        { "MOUSE5", VK_XBUTTON2, ImGuiKey_MouseX2 },
        { "MULTIPLY", VK_MULTIPLY, ImGuiKey_KeypadMultiply },
        { "MWHEEL_DOWN", VK_MWHEELDOWN, ImGuiKey_MouseWheelY },
        { "MWHEEL_UP", VK_MWHEELUP, ImGuiKey_MouseWheelY },
        { "N", 'N', ImGuiKey_N },
        { "NONE", VK_NONE, ImGuiKey_None },
        { "NUMPAD_0", VK_NUMPAD0, ImGuiKey_Keypad0 },
        { "NUMPAD_1", VK_NUMPAD1, ImGuiKey_Keypad1 },
        { "NUMPAD_2", VK_NUMPAD2, ImGuiKey_Keypad2 },
        { "NUMPAD_3", VK_NUMPAD3, ImGuiKey_Keypad3 },
        { "NUMPAD_4", VK_NUMPAD4, ImGuiKey_Keypad4 },
        { "NUMPAD_5", VK_NUMPAD5, ImGuiKey_Keypad5 },
        { "NUMPAD_6", VK_NUMPAD6, ImGuiKey_Keypad6 },
        { "NUMPAD_7", VK_NUMPAD7, ImGuiKey_Keypad7 },
        { "NUMPAD_8", VK_NUMPAD8, ImGuiKey_Keypad8 },
        { "NUMPAD_9", VK_NUMPAD9, ImGuiKey_Keypad9 },
        { "O", 'O', ImGuiKey_O },
        { "P", 'P', ImGuiKey_P },
        { "PAGE_DOWN", VK_NEXT, ImGuiKey_PageDown },
        { "PAGE_UP", VK_PRIOR, ImGuiKey_PageUp },
        { "Q", 'Q', ImGuiKey_Q },
        { "R", 'R', ImGuiKey_R },
        { "RALT", VK_RMENU, ImGuiKey_RightAlt },
        { "RCTRL", VK_RCONTROL, ImGuiKey_RightCtrl },
        { "RIGHT", VK_RIGHT, ImGuiKey_RightArrow },
        { "RSHIFT", VK_RSHIFT, ImGuiKey_RightShift },
        { "S", 'S', ImGuiKey_S },
        { "SPACE", VK_SPACE, ImGuiKey_Space },
        { "SUBTRACT", VK_SUBTRACT, ImGuiKey_KeypadSubtract },
        { "T", 'T', ImGuiKey_T },
        { "TAB", VK_TAB, ImGuiKey_Tab },
        { "U", 'U', ImGuiKey_U },
        { "UP", VK_UP, ImGuiKey_UpArrow },
        { "V", 'V', ImGuiKey_V },
        { "W", 'W', ImGuiKey_W },
        { "X", 'X', ImGuiKey_X },
        { "Y", 'Y', ImGuiKey_Y },
        { "Z", 'Z', ImGuiKey_Z },
        { "[", VK_OEM_4, ImGuiKey_LeftBracket },
        { "\\", VK_OEM_5, ImGuiKey_Backslash },
        { "]", VK_OEM_6, ImGuiKey_RightBracket },
        { "`", VK_OEM_3, ImGuiKey_GraveAccent }
    });
};

namespace Core {
	extern bool exiting;

    extern int rendererAPI;
    extern void OnPostUpdate();
}