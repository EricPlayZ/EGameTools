#include <pch.h>
#include "..\core.h"

namespace ImGui {
    bool isAnyHotkeyBtnPressed = false;
    Utils::Time::Timer timeSinceHotkeyBtnPressed{ 250 };

    void Hotkey(const std::string_view& label, KeyBindOption* key) {
        ImGuiContext& g = *GImGui;
        bool wasDisabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
        if (wasDisabled)
            EndDisabled();

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

        if (GetActiveID() == id) {
            PushStyleColor(ImGuiCol_Button, GetColorU32(ImGuiCol_ButtonActive));
            Button("...", ImVec2(0.0f, 0.0f));
            PopStyleColor();

            GetCurrentContext()->ActiveIdAllowOverlap = true;
            if ((!IsItemHovered() && GetIO().MouseClicked[0]) || key->SetToPressedKey()) {
                timeSinceHotkeyBtnPressed = Utils::Time::Timer(250);
                isAnyHotkeyBtnPressed = false;
                ClearActiveID();
            } else
                SetActiveID(id, GetCurrentWindow());
        } else if (Button(key->ToString(), ImVec2(0.0f, 0.0f))) {
            isAnyHotkeyBtnPressed = true;
            SetActiveID(id, GetCurrentWindow());
        }

        PopID();

        if (wasDisabled)
            BeginDisabled(wasDisabled);
    }
}