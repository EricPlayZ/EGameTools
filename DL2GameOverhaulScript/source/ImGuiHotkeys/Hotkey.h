#pragma once

#include "imgui.h"

class KeyBind;

namespace ImGuiCustom
{
    void hotkey(const char* label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = { 100.0f, 0.0f }) noexcept;
}