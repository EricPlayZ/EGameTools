#pragma once
#include "..\core.h"
#include "..\utils\time.h"

namespace ImGui {
    extern bool isAnyHotkeyBtnPressed;
    extern Utils::Time::Timer timeSinceHotkeyBtnPressed;

    void Hotkey(const std::string_view& label, KeyBindOption* key);
}