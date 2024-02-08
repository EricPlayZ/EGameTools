#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "Hotkey.h"

bool ImGui::isAnyHotkeyBtnPressed = false;
Utils::Timer ImGui::timeSinceHotkeyBtnPressed{ 250 };

void ImGui::Hotkey(const std::string_view& label, KeyBindOption* key) {
    ImGuiContext& g = *GImGui;
    bool wasDisabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (wasDisabled)
        ImGui::EndDisabled();

    const ImGuiID id = ImGui::GetID(label.data());
    ImGui::PushID(label.data());

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
    ImGui::SameLine(0.0f);

    if (ImGui::GetActiveID() == id) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        ImGui::Button("...", ImVec2(0.0f, 0.0f));
        ImGui::PopStyleColor();

        ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
        if ((!ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[0]) || key->SetToPressedKey()) {
            timeSinceHotkeyBtnPressed = Utils::Timer(250);
            isAnyHotkeyBtnPressed = false;
            ImGui::ClearActiveID();
        }
        else
            ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }
    else if (ImGui::Button(key->ToString(), ImVec2(0.0f, 0.0f))) {
        isAnyHotkeyBtnPressed = true;
        ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }

    ImGui::PopID();

    if (wasDisabled)
        ImGui::BeginDisabled(wasDisabled);
}