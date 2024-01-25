#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "Hotkey.h"

bool ImGui::isAnyHotkeyBtnPressed = false;
Utils::Timer ImGui::timeSinceHotkeyBtnPressed{ 250 };

void ImGui::Hotkey(std::string_view label, KeyBindOption& key, float samelineOffset, const ImVec2& size) {
    const auto id = ImGui::GetID(label.data());
    ImGui::PushID(label.data());

    if (!label.contains("##"))
        ImGui::Text("%s", label.data());
    ImGui::SameLine(samelineOffset);

    if (ImGui::GetActiveID() == id) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        ImGui::Button("...", size);
        ImGui::PopStyleColor();

        ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
        if ((!ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[0]) || key.SetToPressedKey()) {
            timeSinceHotkeyBtnPressed = Utils::Timer(250);
            isAnyHotkeyBtnPressed = false;
            ImGui::ClearActiveID();
        }
        else
            ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }
    else if (ImGui::Button(key.ToString(), size)) {
        isAnyHotkeyBtnPressed = true;
        ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }

    ImGui::PopID();
}