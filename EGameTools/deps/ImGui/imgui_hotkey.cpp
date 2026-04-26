#include <Windows.h>
#include <ranges>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imgui_internal.h>
#include <ImGui\imguiex.h>
#include <EGSDK\Utils\Time.h>
#include <EGSDK\Utils\Values.h>
#include <EGSDK\Core\Core.h>

namespace ImGui {
    bool isAnyHotkeyBtnClicked = false;
    EGSDK::Utils::Time::Timer timeSinceHotkeyBtnPressed{ 250 };

    Key::Key(std::string_view name, int code, ImGuiKey imGuiCode) : name(name), code(code), imGuiCode(imGuiCode) {}
    VKey::VKey(std::string_view name, int code) : name(name), code(code) {}

#pragma region Option
    Option::Option(bool value) : value(value), previousValue(value) {
        GetInstances()->insert(this);
    };
    Option::Option(bool value, std::initializer_list<uint32_t> unsupportedGameVers) : value(value), previousValue(value), unsupportedGameVers(unsupportedGameVers) {
        GetInstances()->insert(this);
    };
    Option::Option(bool value, const ConfigBindingInfo& configBinding) : Option(value) {
        SetConfigBinding(configBinding);
    }
    Option::Option(bool value, const ConfigBindingInfo& configBinding, std::initializer_list<uint32_t> unsupportedGameVers) : Option(value, unsupportedGameVers) {
        SetConfigBinding(configBinding);
    }
    Option::~Option() {
        GetInstances()->erase(this);
    }
    std::set<Option*>* Option::GetInstances() {
        static std::set<Option*> instances{};
        return &instances;
    };

    void Option::SetChangesAreDisabled(bool newValue) {
        changesAreDisabled = newValue;
    }
    bool Option::GetChangesAreDisabled() const {
        return changesAreDisabled;
    }

    void Option::Toggle() {
        previousValue = value;
        value = !value;
    }
    void Option::Set(bool newValue) {
        previousValue = value;
        value = newValue;
    }
    void Option::SetBothValues(bool newValue) {
        previousValue = newValue;
        value = newValue;
    }
    void Option::SetValue(bool newValue) {
        value = newValue;
    }
    void Option::SetPrevValue(bool newValue) {
        previousValue = newValue;
    }
    bool Option::GetValue() const {
        return value;
    }
    bool Option::GetPrevValue() const {
        return previousValue;
    }
    bool Option::HasChanged() const {
        return previousValue != value;
    }
    bool Option::HasChangedTo(bool toValue) const {
        return previousValue != value && value == toValue;
    }
    void Option::SetConfigBinding(const ConfigBindingInfo& configBinding) {
        if (!configBinding.IsValid()) {
            configSection.clear();
            configKey.clear();
            return;
        }

        configSection = configBinding.section;
        configKey = configBinding.key;
    }
    bool Option::HasConfigBinding() const {
        return !configSection.empty() && !configKey.empty();
    }
    std::string_view Option::GetConfigSection() const {
        return configSection;
    }
    std::string_view Option::GetConfigKey() const {
        return configKey;
    }

    uint32_t Option::IsUnsupportedGameVer() const {
        auto it = unsupportedGameVers.find(EGSDK::Core::gameVer);
        return it != unsupportedGameVers.end() ? EGSDK::Core::gameVer : 0;
    }
#pragma endregion

#pragma region KeyBindOption
    bool KeyBindOption::wasAnyHotkeyToggled = false;
    bool KeyBindOption::scrolledMouseWheelUp = false;
    bool KeyBindOption::scrolledMouseWheelDown = false;

    KeyBindOption::KeyBindOption(bool value, int keyCode, bool isToggleableOption, std::initializer_list<uint32_t> unsupportedGameVers) : keyCode(keyCode), isToggleableOption(isToggleableOption), Option(value, unsupportedGameVers) {
        keyBindBehavior = isToggleableOption ? KeyBindBehavior::ToggleOption : KeyBindBehavior::Action;
        GetInstances()->insert(this);
    };
    KeyBindOption::KeyBindOption(bool value, int keyCode, const ConfigBindingInfo& configBinding, bool isToggleableOption, std::initializer_list<uint32_t> unsupportedGameVers)
        : keyCode(keyCode), isToggleableOption(isToggleableOption), Option(value, unsupportedGameVers) {
        SetKeyBindConfigBinding(configBinding);
        keyBindBehavior = configBinding.keyBindBehavior;
        GetInstances()->insert(this);
    };
    KeyBindOption::KeyBindOption(bool value, int keyCode, const ConfigBindingInfo& keyConfigBinding, const ConfigBindingInfo& optionConfigBinding, bool isToggleableOption, std::initializer_list<uint32_t> unsupportedGameVers)
        : keyCode(keyCode), isToggleableOption(isToggleableOption), Option(value, optionConfigBinding, unsupportedGameVers) {
        SetKeyBindConfigBinding(keyConfigBinding);
        keyBindBehavior = keyConfigBinding.keyBindBehavior;
        GetInstances()->insert(this);
    };
    KeyBindOption::~KeyBindOption() {
        GetInstances()->erase(this);
    }
    std::set<KeyBindOption*>* KeyBindOption::GetInstances() {
        static std::set<KeyBindOption*> instances{};
        return &instances;
    };

    std::string KeyBindOption::ToStringKeyMap() {
        if (const auto it = std::ranges::find(keyMap, keyCode, &Key::code); it != keyMap.end())
            return it->name.data();
        return {};
    }
    std::string KeyBindOption::ToStringVKeyMap() {
        if (const auto it = std::ranges::find(virtualKeyMap, keyCode, &VKey::code); it != virtualKeyMap.end())
            return it->name.data();
        return {};
    }
    std::string KeyBindOption::ToStringKeyMap(int keyCode) {
        if (const auto it = std::ranges::find(keyMap, keyCode, &Key::code); it != keyMap.end())
            return it->name.data();
        return {};
    }
    std::string KeyBindOption::ToStringVKeyMap(int keyCode) {
        if (const auto it = std::ranges::find(virtualKeyMap, keyCode, &VKey::code); it != virtualKeyMap.end())
            return it->name.data();
        return {};
    }
    int KeyBindOption::ToKeyCodeKeyMap(const std::string& keyName) {
        if (const auto it = std::ranges::find(keyMap, keyName, &Key::name); it != keyMap.end())
            return it->code;
        return VK_INVALID;
    }
    int KeyBindOption::ToKeyCodeVKeyMap(const std::string& keyName) {
        if (const auto it = std::ranges::find(virtualKeyMap, keyName, &VKey::name); it != virtualKeyMap.end())
            return it->code;
        return VK_INVALID;
    }
    int KeyBindOption::GetKeyBind() const {
        return keyCode;
    }
    void KeyBindOption::ChangeKeyBind(int newKeyBind) {
        keyCode = newKeyBind;
    }

    bool KeyBindOption::SetToPressedKey() {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ChangeKeyBind(VK_NONE);
            return true;
        } else if (!EGSDK::Utils::Values::are_samef(ImGui::GetIO().MouseWheel, 0.0f)) {
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
    bool KeyBindOption::IsToggleableOption() const {
        return isToggleableOption;
    }
    KeyBindBehavior KeyBindOption::GetKeyBindBehavior() const {
        return keyBindBehavior;
    }
    void KeyBindOption::SetKeyBindConfigBinding(const ConfigBindingInfo& configBinding) {
        if (!configBinding.IsValid()) {
            keyBindConfigSection.clear();
            keyBindConfigKey.clear();
            return;
        }

        keyBindConfigSection = configBinding.section;
        keyBindConfigKey = configBinding.key;
    }
    bool KeyBindOption::HasKeyBindConfigBinding() const {
        return !keyBindConfigSection.empty() && !keyBindConfigKey.empty();
    }
    std::string_view KeyBindOption::GetKeyBindConfigSection() const {
        return keyBindConfigSection;
    }
    std::string_view KeyBindOption::GetKeyBindConfigKey() const {
        return keyBindConfigKey;
    }

    void KeyBindOption::SetIsKeyDown(bool newValue) {
        isKeyDown = newValue;
    }
    void KeyBindOption::SetIsKeyPressed(bool newValue) {
        isKeyPressed = newValue;
    }
    void KeyBindOption::SetIsKeyReleased(bool newValue) {
        isKeyReleased = newValue;
    }
    bool KeyBindOption::IsKeyDown() {
        return isKeyDown;
    }
    bool KeyBindOption::IsKeyPressed() {
        if (isKeyPressed) {
            isKeyPressed = false;
            return true;
        }
        return false;
    }
    bool KeyBindOption::IsKeyReleased() {
        if (!isKeyDown && isKeyReleased) {
            isKeyReleased = false;
            return true;
        }
        return false;
    }

    const std::array<Key, 103> KeyBindOption::keyMap = std::to_array<Key>({
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
    const std::array<VKey, 135> KeyBindOption::virtualKeyMap = std::to_array<VKey>({
        { "VK_NONE", VK_NONE },

        // Function keys
        { "VK_F1", VK_F1 },
        { "VK_F2", VK_F2 },
        { "VK_F3", VK_F3 },
        { "VK_F4", VK_F4 },
        { "VK_F5", VK_F5 },
        { "VK_F6", VK_F6 },
        { "VK_F7", VK_F7 },
        { "VK_F8", VK_F8 },
        { "VK_F9", VK_F9 },
        { "VK_F10", VK_F10 },
        { "VK_F11", VK_F11 },
        { "VK_F12", VK_F12 },

        // Number keys
        { "VK_0", '0' },
        { "VK_1", '1' },
        { "VK_2", '2' },
        { "VK_3", '3' },
        { "VK_4", '4' },
        { "VK_5", '5' },
        { "VK_6", '6' },
        { "VK_7", '7' },
        { "VK_8", '8' },
        { "VK_9", '9' },
        { "0", '0' },
        { "1", '1' },
        { "2", '2' },
        { "3", '3' },
        { "4", '4' },
        { "5", '5' },
        { "6", '6' },
        { "7", '7' },
        { "8", '8' },
        { "9", '9' },

        // Alphabetic keys
        { "VK_A", 'A' },
        { "VK_B", 'B' },
        { "VK_C", 'C' },
        { "VK_D", 'D' },
        { "VK_E", 'E' },
        { "VK_F", 'F' },
        { "VK_G", 'G' },
        { "VK_H", 'H' },
        { "VK_I", 'I' },
        { "VK_J", 'J' },
        { "VK_K", 'K' },
        { "VK_L", 'L' },
        { "VK_M", 'M' },
        { "VK_N", 'N' },
        { "VK_O", 'O' },
        { "VK_P", 'P' },
        { "VK_Q", 'Q' },
        { "VK_R", 'R' },
        { "VK_S", 'S' },
        { "VK_T", 'T' },
        { "VK_U", 'U' },
        { "VK_V", 'V' },
        { "VK_W", 'W' },
        { "VK_X", 'X' },
        { "VK_Y", 'Y' },
        { "VK_Z", 'Z' },
        { "A", 'A' },
        { "B", 'B' },
        { "C", 'C' },
        { "D", 'D' },
        { "E", 'E' },
        { "F", 'F' },
        { "G", 'G' },
        { "H", 'H' },
        { "I", 'I' },
        { "J", 'J' },
        { "K", 'K' },
        { "L", 'L' },
        { "M", 'M' },
        { "N", 'N' },
        { "O", 'O' },
        { "P", 'P' },
        { "Q", 'Q' },
        { "R", 'R' },
        { "S", 'S' },
        { "T", 'T' },
        { "U", 'U' },
        { "V", 'V' },
        { "W", 'W' },
        { "X", 'X' },
        { "Y", 'Y' },
        { "Z", 'Z' },

        // Special keys
        { "VK_BACK", VK_BACK },
        { "VK_TAB", VK_TAB },
        { "VK_RETURN", VK_RETURN },
        { "VK_CAPITAL", VK_CAPITAL },
        { "VK_SPACE", VK_SPACE },
        { "VK_PRIOR", VK_PRIOR },
        { "VK_NEXT", VK_NEXT },
        { "VK_END", VK_END },
        { "VK_HOME", VK_HOME },
        { "VK_LEFT", VK_LEFT },
        { "VK_UP", VK_UP },
        { "VK_RIGHT", VK_RIGHT },
        { "VK_DOWN", VK_DOWN },
        { "VK_INSERT", VK_INSERT },
        { "VK_DELETE", VK_DELETE },

        // Numpad keys
        { "VK_NUMPAD0", VK_NUMPAD0 },
        { "VK_NUMPAD1", VK_NUMPAD1 },
        { "VK_NUMPAD2", VK_NUMPAD2 },
        { "VK_NUMPAD3", VK_NUMPAD3 },
        { "VK_NUMPAD4", VK_NUMPAD4 },
        { "VK_NUMPAD5", VK_NUMPAD5 },
        { "VK_NUMPAD6", VK_NUMPAD6 },
        { "VK_NUMPAD7", VK_NUMPAD7 },
        { "VK_NUMPAD8", VK_NUMPAD8 },
        { "VK_NUMPAD9", VK_NUMPAD9 },
        { "VK_MULTIPLY", VK_MULTIPLY },
        { "VK_ADD", VK_ADD },
        { "VK_SUBTRACT", VK_SUBTRACT },
        { "VK_DECIMAL", VK_DECIMAL },
        { "VK_DIVIDE", VK_DIVIDE },

        // Modifier keys
        { "VK_SHIFT", VK_LSHIFT },
        { "VK_LSHIFT", VK_LSHIFT },
        { "VK_RSHIFT", VK_RSHIFT },
        { "VK_CONTROL", VK_LCONTROL },
        { "VK_LCONTROL", VK_LCONTROL },
        { "VK_RCONTROL", VK_RCONTROL },
        { "VK_MENU", VK_LMENU },
        { "VK_LMENU", VK_LMENU },
        { "VK_RMENU", VK_RMENU },

        // Other keys
        { "VK_OEM_1", VK_OEM_1 },
        { "VK_OEM_PLUS", VK_OEM_PLUS },
        { "VK_OEM_COMMA", VK_OEM_COMMA },
        { "VK_OEM_MINUS", VK_OEM_MINUS },
        { "VK_OEM_PERIOD", VK_OEM_PERIOD },
        { "VK_OEM_2", VK_OEM_2 },
        { "VK_OEM_3", VK_OEM_3 },
        { "VK_OEM_4", VK_OEM_4 },
        { "VK_OEM_5", VK_OEM_5 },
        { "VK_OEM_6", VK_OEM_6 },
        { "VK_OEM_7", VK_OEM_7 }
    });
#pragma endregion

    void Hotkey(const std::string_view& label, KeyBindOption* key) {
        if (key->IsUnsupportedGameVer())
            ImGui::BeginDisabled(key->IsUnsupportedGameVer());

        ImGuiContext& g = *GImGui;
        bool wasDisabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;

        ImGuiItemFlags backupFlags = g.CurrentItemFlags;
        float backupAlpha = g.Style.Alpha;

        if (wasDisabled && !key->IsUnsupportedGameVer()) {
            ImGui::EndDisabled();
            g.CurrentItemFlags &= ~ImGuiItemFlags_Disabled;
            g.Style.Alpha = backupAlpha / g.Style.DisabledAlpha;
        }

        const ImGuiID id = GetID(label.data());
        PushID(label.data());

        ImGuiWindow* window = GetCurrentWindow();

        if (!label.contains("##") && !window->SkipItems) {
            const ImGuiStyle& style = (*GImGui).Style;

            const ImVec2 label_size = CalcTextSize(label.data(), nullptr, true);
            const ImVec2 pos = window->DC.CursorPos;
            const ImRect total_bb(pos, pos + ImVec2((label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
            ItemSize(total_bb, style.FramePadding.y);

            if (ItemAdd(total_bb, window->GetID(label.data()))) {
                const ImVec2 label_pos = ImVec2(pos.x + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y);
                if (label_size.x > 0.0f)
                    RenderText(label_pos, label.data());
            }
        }
        SameLine(0.0f);

        const auto hotkeyBtnWidth = [&](const char* txt) -> float {
            if (window->SkipItems)
                return 0.0f;
            const float avail = GetContentRegionAvail().x;
            const ImVec2 ts = CalcTextSize(txt, nullptr, true);
            const float want = ts.x + g.Style.FramePadding.x * 2.0f;
            return (avail > 1.0f) ? ImMin(want, avail) : want;
        };

        if (GetActiveID() == id) {
            PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
            Button("...", ImVec2(hotkeyBtnWidth("..."), 0.0f));
            PopStyleColor();

            GetCurrentContext()->ActiveIdAllowOverlap = true;
            if ((!IsItemHovered() && GetIO().MouseClicked[0]) || key->SetToPressedKey()) {
                timeSinceHotkeyBtnPressed = EGSDK::Utils::Time::Timer(250);
                isAnyHotkeyBtnClicked = false;
                ClearActiveID();
            } else
                SetActiveID(id, GetCurrentWindow());
        } else if (ButtonSmooth(key->ToStringKeyMap().data(), ImVec2(hotkeyBtnWidth(key->ToStringKeyMap().data()), 0.0f))) {
            isAnyHotkeyBtnClicked = true;
            SetActiveID(id, GetCurrentWindow());
        }

        PopID();

        if (wasDisabled && !key->IsUnsupportedGameVer()) {
            g.CurrentItemFlags = backupFlags;
            g.Style.Alpha = backupAlpha;
            ImGui::BeginDisabled(true);
        }

        if (key->IsUnsupportedGameVer())
            ImGui::EndDisabled();
    }
}