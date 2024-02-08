#pragma once
#include <imgui_internal.h>
#include <string_view>
#include <unordered_map>

namespace ImGui {
    extern std::unordered_map<std::string_view, float> AnimValueStack;

    const float AnimEaseInSine(const float blendPoint);
    const float AnimEaseOutSine(const float blendPoint);

    extern const float AnimateLerp(const std::string_view& valueName, const float a, const float b, const float animSpeed, const bool resetBlendPoint = false, const float (*easingFunction)(float) = nullptr);
}