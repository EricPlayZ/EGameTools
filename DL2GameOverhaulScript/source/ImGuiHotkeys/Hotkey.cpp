#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "Hotkey.h"
#include "InputUtil.h"

void ImGuiCustom::hotkey(const char* label, KeyBind& key, float samelineOffset, const ImVec2& size) noexcept
{
    const auto id = ImGui::GetID(label);
    ImGui::PushID(label);

    ImGui::TextUnformatted(label);
    ImGui::SameLine(samelineOffset);

    if (ImGui::GetActiveID() == id) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        ImGui::Button("...", size);
        ImGui::PopStyleColor();

        ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
        if ((!ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[0]) || key.setToPressedKey())
            ImGui::ClearActiveID();
    }
    else if (ImGui::Button(key.toString(), size)) {
        ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }

    ImGui::PopID();
}