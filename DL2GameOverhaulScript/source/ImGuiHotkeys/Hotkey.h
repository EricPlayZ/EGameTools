#pragma once
#include <imgui.h>
#include <InputUtil.h>
#include "../utils.h"

namespace ImGui
{
    extern bool isAnyHotkeyBtnPressed;
    extern Utils::Timer timeSinceHotkeyBtnPressed;

    void Hotkey(std::string_view label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = {0.0f, 0.0f}) noexcept;
}