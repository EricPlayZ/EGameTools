#define IMGUI_DEFINE_MATH_OPERATORS
#include <pch.h>
#include "..\core.h"

namespace ImGui {
    static ImGuiStyle defImGuiStyle{};
    static size_t tabIndex = 1;

    void StyleScaleAllSizes(ImGuiStyle* style, const float scale_factor, ImGuiStyle* defStyle) {
        if (!defStyle)
            defStyle = &defImGuiStyle;

        style->WindowPadding = ImFloor(defStyle->WindowPadding * scale_factor);
        style->WindowRounding = ImFloor(defStyle->WindowRounding * scale_factor);
        style->WindowMinSize = ImFloor(defStyle->WindowMinSize * scale_factor);
        style->ChildRounding = ImFloor(defStyle->ChildRounding * scale_factor);
        style->PopupRounding = ImFloor(defStyle->PopupRounding * scale_factor);
        style->FramePadding = ImFloor(defStyle->FramePadding * scale_factor);
        style->FrameRounding = ImFloor(defStyle->FrameRounding * scale_factor);
        style->ItemSpacing = ImFloor(defStyle->ItemSpacing * scale_factor);
        style->ItemInnerSpacing = ImFloor(defStyle->ItemInnerSpacing * scale_factor);
        style->CellPadding = ImFloor(defStyle->CellPadding * scale_factor);
        style->TouchExtraPadding = ImFloor(defStyle->TouchExtraPadding * scale_factor);
        style->IndentSpacing = ImFloor(defStyle->IndentSpacing * scale_factor);
        style->ColumnsMinSpacing = ImFloor(defStyle->ColumnsMinSpacing * scale_factor);
        style->ScrollbarSize = ImFloor(defStyle->ScrollbarSize * scale_factor);
        style->ScrollbarRounding = ImFloor(defStyle->ScrollbarRounding * scale_factor);
        style->GrabMinSize = ImFloor(defStyle->GrabMinSize * scale_factor);
        style->GrabRounding = ImFloor(defStyle->GrabRounding * scale_factor);
        style->LogSliderDeadzone = ImFloor(defStyle->LogSliderDeadzone * scale_factor);
        style->TabRounding = ImFloor(defStyle->TabRounding * scale_factor);
        style->TabMinWidthForCloseButton = (defStyle->TabMinWidthForCloseButton != FLT_MAX) ? ImFloor(defStyle->TabMinWidthForCloseButton * scale_factor) : FLT_MAX;
        style->SeparatorTextPadding = ImFloor(defStyle->SeparatorTextPadding * scale_factor);
        style->DisplayWindowPadding = ImFloor(defStyle->DisplayWindowPadding * scale_factor);
        style->DisplaySafeAreaPadding = ImFloor(defStyle->DisplaySafeAreaPadding * scale_factor);
        style->MouseCursorScale = ImFloor(defStyle->MouseCursorScale * scale_factor);
    }
    void SpanNextTabAcrossWidth(const float width, const size_t tabs) {
        if (width <= 0.0f)
            return;

        const float oneTabWidthWithSpacing = width / tabs;
        const float oneTabWidth = oneTabWidthWithSpacing - (tabIndex == tabs ? 0.0f : GImGui->Style.ItemSpacing.x / 2.0f);
        SetNextItemWidth(oneTabWidth);
    }
    void EndTabBarEx() {
        EndTabBar();
        tabIndex = 1;
    }
    bool Button(const char* label, const char* tooltip, const ImVec2& size) {
        bool btn = Button(label, size);
        if (tooltip)
            SetItemTooltip(tooltip);
        return btn;
    }
    bool ButtonHotkey(const char* label, KeyBindOption* v, const char* tooltip, const ImVec2& size) {
        bool btn = Button(label, size);
        if (tooltip)
            SetItemTooltip(tooltip);
        Hotkey(std::string(label + std::string("##ToggleKey")), v);
        return btn;
    }
    bool Checkbox(const char* label, bool* v, const char* tooltip) {
        bool checkbox = Checkbox(label, v);
        SetItemTooltip(tooltip);
        return checkbox;
    }
	bool Checkbox(const char* label, Option* v) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        const float square_sz = GetFrameHeight();
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id)) {
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (v->GetValue() ? ImGuiItemStatusFlags_Checked : 0));
            return false;
        }

        bool hovered, held;
        bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) {
            v->Toggle();
            MarkItemEdited(id);
        }

        const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
        RenderNavHighlight(total_bb, id);
        RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
        ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
        bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
        if (mixed_value) {
            // Undocumented tristate/mixed/indeterminate checkbox (#2644)
            // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
            ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
        } else if (v->GetValue()) {
            const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
            RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
        }

        ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
        if (g.LogEnabled)
            LogRenderedText(&label_pos, mixed_value ? "[~]" : v->GetValue() ? "[x]" : "[ ]");
        if (label_size.x > 0.0f)
            RenderText(label_pos, label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (v->GetValue() ? ImGuiItemStatusFlags_Checked : 0));
        return pressed;
	}
    bool Checkbox(const char* label, Option* v, const char* tooltip) {
        bool checkbox = Checkbox(label, v);
        SetItemTooltip(tooltip);
        return checkbox;
    }
    bool CheckboxHotkey(const char* label, KeyBindOption* v, const char* tooltip) {
        const bool checkbox = Checkbox(label, v);
        if (tooltip)
            SetItemTooltip(tooltip);
        Hotkey(std::string(label + std::string("##ToggleKey")), v);
        return checkbox;
    }
    bool SliderInt(const char* label, const char* tooltip, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
        const bool sliderInt = SliderInt(label, v, v_min, v_max, format, flags);
        if (tooltip)
            SetItemTooltip(tooltip);
        return sliderInt;
    }
    bool SliderFloat(const char* label, const char* tooltip, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        const bool sliderFloat = SliderFloat(label, v, v_min, v_max, format, flags);
        if (tooltip)
            SetItemTooltip(tooltip);
        return sliderFloat;
    }
    static const float CalculateIndentation(const float window_width, const float text_width, const float min_indentation) {
        const float indentation = (window_width - text_width) * 0.5f;
        return (indentation > min_indentation ? indentation : min_indentation);
    }
    void TextCentered(const char* text, const bool calculateWithScrollbar) {
        const float min_indentation = 20.0f;
        const float window_width = GetWindowSize().x - (calculateWithScrollbar ? GImGui->Style.ScrollbarSize : 0.0f) - (GImGui->Style.WindowPadding.x);
        const float wrap_pos = window_width - min_indentation;

        std::istringstream iss(text);
        std::vector<std::string> words((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

        std::string line{};
        for (const auto& word : words) {
            std::string new_line = line.empty() ? word : line + " " + word;
            if (CalcTextSize(new_line.c_str()).x <= wrap_pos) {
                line = new_line;
                continue;
            }

            SameLine(CalculateIndentation(window_width, CalcTextSize(line.c_str()).x, min_indentation));
            TextUnformatted(line.c_str());
            NewLine();
            line = word;
        }

        if (!line.empty()) {
            SameLine(CalculateIndentation(window_width, CalcTextSize(line.c_str()).x, min_indentation));
            TextUnformatted(line.c_str());
            NewLine();
        }
    }
    void TextCenteredColored(const char* text, const ImU32 col, const bool calculateWithScrollbar) {
        const float min_indentation = 20.0f;
        const float window_width = GetWindowSize().x - (calculateWithScrollbar ? GImGui->Style.ScrollbarSize : 0.0f) - (GImGui->Style.WindowPadding.x);
        const float wrap_pos = window_width - min_indentation;

        std::istringstream iss(text);
        std::vector<std::string> words((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

        std::string line{};
        for (const auto& word : words) {
            std::string new_line = line.empty() ? word : line + " " + word;
            if (CalcTextSize(new_line.c_str()).x <= wrap_pos) {
                line = new_line;
                continue;
            }

            SameLine(CalculateIndentation(window_width, CalcTextSize(line.c_str()).x, min_indentation));
            PushStyleColor(ImGuiCol_Text, col);
            TextUnformatted(line.c_str());
            PopStyleColor();
            NewLine();
            line = word;
        }

        if (!line.empty()) {
            SameLine(CalculateIndentation(window_width, CalcTextSize(line.c_str()).x, min_indentation));
            PushStyleColor(ImGuiCol_Text, col);
            TextUnformatted(line.c_str());
            PopStyleColor();
            NewLine();
        }
    }
    bool ButtonCentered(const char* label, const ImVec2 size) {
        ImGuiStyle& style = GetStyle();
        const float window_width = GetContentRegionAvail().x;
        const float button_width = CalcTextSize(label).x + style.FramePadding.x * 2.0f;
        float button_indentation = (window_width - button_width) * 0.5f;

        const float min_indentation = 20.0f;
        if (button_indentation <= min_indentation)
            button_indentation = min_indentation;

        SameLine(button_indentation);
        return Button(label, size);
        NewLine();
    }
    void SeparatorTextColored(const char* label, const ImU32 col) {
        PushStyleColor(ImGuiCol_Text, col);
        SeparatorText(label);
        PopStyleColor();
    }
    void Spacing(const ImVec2 size, const bool customPosOffset) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return;

        if (!customPosOffset) {
            ItemSize(size);
            return;
        }

        const ImVec2 currentCursorPos = GetCursorPos();
        if (size.y == 0.0f)
            SetCursorPosX(currentCursorPos.x + size.x);
        else if (size.x == 0.0f)
            SetCursorPosY(currentCursorPos.y + size.y);
        else
            SetCursorPos(currentCursorPos + size);
    }
}