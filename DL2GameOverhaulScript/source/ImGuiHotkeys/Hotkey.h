#pragma once
#include <imgui.h>
#include "..\..\core.h"
#include "..\utils.h"

namespace ImGui {
    extern bool isAnyHotkeyBtnPressed;
    extern Utils::Timer timeSinceHotkeyBtnPressed;

    void Hotkey(const std::string_view& label, KeyBindOption* key);
}