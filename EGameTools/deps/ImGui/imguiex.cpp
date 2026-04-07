#include <cmath>
#include <cstdarg>
#include <unordered_map>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imgui_internal.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
namespace ImGui {
    static ImGuiStyle defImGuiStyle{};
    static size_t tabIndex = 1;

    /// Defined below; used by helpers in the anonymous namespace before this TU's definition line.
    void SetItemTooltipAnimated(const char* fmt, ...);

    /// Check path matches `RenderCheckMark`; `t` in [0,1] is distance along the stroke (draw + uncheck retract).
    /// Single `PathStroke` so the bend gets a proper join (avoids gaps from two `AddLine` calls).
    static void RenderCheckMarkProgress(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz, float t) {
        t = ImClamp(t, 0.0f, 1.0f);
        if (t <= 0.0f)
            return;
        const float thickness = ImMax(sz / 5.0f, 1.0f);
        const float inner = sz - thickness * 0.5f;
        pos += ImVec2(thickness * 0.25f, thickness * 0.25f);
        const float third = inner / 3.0f;
        const float bx = pos.x + third;
        const float by = pos.y + inner - third * 0.5f;
        const ImVec2 p0(bx - third, by - third);
        const ImVec2 p1(bx, by);
        const ImVec2 p2(bx + third * 2.0f, by - third * 2.0f);
        float dx = p1.x - p0.x, dy = p1.y - p0.y;
        const float d01 = ImSqrt(dx * dx + dy * dy);
        dx = p2.x - p1.x;
        dy = p2.y - p1.y;
        const float d12 = ImSqrt(dx * dx + dy * dy);
        const float total = d01 + d12;
        if (total < 1.0f)
            return;
        const float dist = t * total;
        draw_list->PathClear();
        draw_list->PathLineTo(p0);
        if (dist <= d01 + 1e-5f) {
            const float u = ImClamp(dist / d01, 0.0f, 1.0f);
            draw_list->PathLineTo(ImVec2(p0.x + (p1.x - p0.x) * u, p0.y + (p1.y - p0.y) * u));
        } else {
            draw_list->PathLineTo(p1);
            const float u = ImClamp((dist - d01) / d12, 0.0f, 1.0f);
            draw_list->PathLineTo(ImVec2(p1.x + (p2.x - p1.x) * u, p1.y + (p2.y - p1.y) * u));
        }
        draw_list->PathStroke(col, 0, thickness);
    }

    /// Visible label (before ##) on the left; call before SameLine control. Optional tooltip on the label.
    static void InlineFormLabel(const char* label, const char* tooltip) {
        ImGuiWindow* window = GetCurrentWindow();
        if (!window || window->SkipItems)
            return;
        const char* label_end = FindRenderedTextEnd(label);
        if (label_end <= label)
            return;
        AlignTextToFramePadding();
        TextUnformatted(label, label_end);
        if (tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        SameLine(0.0f, GetStyle().ItemInnerSpacing.x);
    }

    static void SetNextItemWidthRemainder() {
        const float w = GetContentRegionAvail().x;
        SetNextItemWidth(w > 1.0f ? w : -1.0f);
    }

    bool SliderFloatStacked(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, const char* tooltip) {
        PushID(label);
        InlineFormLabel(label, tooltip);
        SetNextItemWidthRemainder();
        const bool ch = SliderFloat("##stk", v, v_min, v_max, format, flags);
        if (FindRenderedTextEnd(label) <= label && tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        PopID();
        return ch;
    }

    bool SliderFloat3Stacked(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        PushID(label);
        InlineFormLabel(label, nullptr);
        SetNextItemWidthRemainder();
        const bool ch = SliderFloat3("##sf3", v, v_min, v_max, format, flags);
        PopID();
        return ch;
    }

    bool SliderIntStacked(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const char* tooltip) {
        PushID(label);
        InlineFormLabel(label, tooltip);
        SetNextItemWidthRemainder();
        const bool ch = SliderInt("##stk", v, v_min, v_max, format, flags);
        if (FindRenderedTextEnd(label) <= label && tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        PopID();
        return ch;
    }

    bool DragIntStacked(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
        PushID(label);
        InlineFormLabel(label, nullptr);
        SetNextItemWidthRemainder();
        const bool ch = DragInt("##dstk", v, v_speed, v_min, v_max, format, flags);
        PopID();
        return ch;
    }

    bool ComboStacked(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items) {
        PushID(label);
        InlineFormLabel(label, nullptr);
        SetNextItemWidthRemainder();
        const bool ch = Combo("##cb", current_item, items, items_count, popup_max_height_in_items);
        PopID();
        return ch;
    }

    bool InputFloat3Stacked(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags) {
        PushID(label);
        InlineFormLabel(label, nullptr);
        SetNextItemWidthRemainder();
        const bool ch = InputFloat3("##if3", v, format, flags);
        PopID();
        return ch;
    }

    static void FormatString(char* buffer, size_t bufferSize, const char* fmt, va_list args) {
        vsnprintf(buffer, bufferSize, fmt, args);
    }

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

    void SetItemTooltipAnimated(const char* fmt, ...) {
        ImGuiContext& imguiContext = *GImGui;
        if (!IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            return;
        const float delay = imguiContext.Style.HoverDelayShort + imguiContext.Style.HoverStationaryDelay * 0.35f;
        float fade = ImClamp((imguiContext.HoveredIdTimer - delay) / 0.1f, 0.0f, 1.0f);
        fade = fade * fade * (3.0f - 2.0f * fade);
        if (fade < 0.02f)
            return;
        va_list args;
        va_start(args, fmt);
        BeginTooltip();
        PushStyleVar(ImGuiStyleVar_Alpha, GetStyle().Alpha * fade);
        TextV(fmt, args);
        PopStyleVar();
        EndTooltip();
        va_end(args);
    }

    bool ButtonSmooth(const char* label, const ImVec2& size_arg) {
        ImGuiContext& imguiContext = *GImGui;
        PushID(label);
        const ImGuiID smoothId = GetID("##smb");
        const int slot = static_cast<int>(smoothId & 127);
        static float buttonBlendBySlot[128]{};
        static bool buttonWasHoveredBySlot[128]{};
        static bool buttonWasHeldBySlot[128]{};
        const float dt = ImMin(imguiContext.IO.DeltaTime, 0.08f);
        const float smoothingStep = 1.0f - std::exp(-22.0f * dt);
        const bool wasHot = buttonWasHoveredBySlot[slot] || buttonWasHeldBySlot[slot];
        const float target = wasHot ? 1.0f : 0.0f;
        float& buttonBlend = buttonBlendBySlot[slot];
        buttonBlend += (target - buttonBlend) * smoothingStep;
        const ImVec4 b = GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 h = GetStyleColorVec4(ImGuiCol_ButtonHovered);
        const ImVec4 a = GetStyleColorVec4(ImGuiCol_ButtonActive);
        const ImVec4 mid(ImLerp(b.x, h.x, buttonBlend), ImLerp(b.y, h.y, buttonBlend), ImLerp(b.z, h.z, buttonBlend), ImLerp(b.w, h.w, buttonBlend));
        PushStyleColor(ImGuiCol_Button, mid);
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImLerp(mid.x, a.x, buttonBlend * 0.4f), ImLerp(mid.y, a.y, buttonBlend * 0.4f), ImLerp(mid.z, a.z, buttonBlend * 0.4f), ImLerp(mid.w, a.w, buttonBlend * 0.4f)));
        PushStyleColor(ImGuiCol_ButtonActive, a);
        const bool pressed = Button(label, size_arg);
        PopStyleColor(3);
        PopID();
        buttonWasHoveredBySlot[slot] = IsItemHovered();
        buttonWasHeldBySlot[slot] = IsItemActive();
        return pressed;
    }

    bool Button(const char* label, const char* tooltip, const ImVec2& size) {
        bool btn = ButtonSmooth(label, size);
        if (tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        return btn;
    }
    bool ButtonHotkey(const char* label, KeyBindOption* v, const char* tooltip, const ImVec2& size) {
        bool btn = ButtonSmooth(label, size);
        if (tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        Hotkey(std::string(label + std::string("##ToggleKey")), v);
        return btn;
    }
    bool Checkbox(const char* label, bool* v, const char* tooltip) {
        bool checkbox = Checkbox(label, v);
        if (tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        return checkbox;
    }

    static ImVec4 BuildCheckboxFrameColor(ImGuiID id, bool hovered, bool held, ImGuiContext& imguiContext) {
        static std::unordered_map<ImGuiID, float> checkboxFrameBlendById;
        static std::unordered_map<ImGuiID, unsigned int> checkboxFrameLastDrawFrameById;

        const unsigned int frameCount = static_cast<unsigned int>(imguiContext.FrameCount);
        const auto lastFrameIt = checkboxFrameLastDrawFrameById.find(id);
        const bool wasDrawnLastFrame = (lastFrameIt != checkboxFrameLastDrawFrameById.end() && lastFrameIt->second + 1u == frameCount);
        checkboxFrameLastDrawFrameById[id] = frameCount;

        const float deltaSeconds = ImMin(imguiContext.IO.DeltaTime, 0.08f);
        const float frameSmoothingStep = 1.0f - std::exp(-20.0f * deltaSeconds);
        const float targetBlend = (held && hovered) ? 1.0f : (hovered ? 0.68f : 0.0f);

        float& frameBlend = checkboxFrameBlendById[id];
        if (!wasDrawnLastFrame)
            frameBlend = targetBlend;
        else
            frameBlend += (targetBlend - frameBlend) * frameSmoothingStep;

        const ImVec4 frameBase = GetStyleColorVec4(ImGuiCol_FrameBg);
        const ImVec4 frameHover = GetStyleColorVec4(ImGuiCol_FrameBgHovered);
        const ImVec4 frameActive = GetStyleColorVec4(ImGuiCol_FrameBgActive);
        ImVec4 frameColor = ImVec4(
            ImLerp(frameBase.x, frameHover.x, frameBlend),
            ImLerp(frameBase.y, frameHover.y, frameBlend),
            ImLerp(frameBase.z, frameHover.z, frameBlend),
            ImLerp(frameBase.w, frameHover.w, frameBlend));
        if (held && hovered) {
            frameColor = ImVec4(
                ImLerp(frameColor.x, frameActive.x, 0.85f),
                ImLerp(frameColor.y, frameActive.y, 0.85f),
                ImLerp(frameColor.z, frameActive.z, 0.85f),
                ImLerp(frameColor.w, frameActive.w, 0.85f));
        }
        return frameColor;
    }

    static void DrawCheckboxMarkAnimated(ImGuiWindow* window, const ImRect& checkboxRect, ImGuiID id, bool isChecked, ImGuiContext& imguiContext) {
        const ImU32 checkColor = GetColorU32(ImGuiCol_CheckMark);
        static std::unordered_map<ImGuiID, float> checkmarkStrokeBlendById;
        static std::unordered_map<ImGuiID, unsigned int> checkmarkStrokeLastDrawFrameById;
        const unsigned int frameCount = static_cast<unsigned int>(imguiContext.FrameCount);
        const auto lastFrameIt = checkmarkStrokeLastDrawFrameById.find(id);
        const bool wasDrawnLastFrame = (lastFrameIt != checkmarkStrokeLastDrawFrameById.end() && lastFrameIt->second + 1u == frameCount);
        checkmarkStrokeLastDrawFrameById[id] = frameCount;
        const float deltaSeconds = ImMin(imguiContext.IO.DeltaTime, 0.08f);
        const float checkmarkSmoothingStep = 1.0f - std::exp(-16.0f * deltaSeconds);
        float& checkmarkStrokeBlend = checkmarkStrokeBlendById[id];
        const float checkmarkTargetBlend = isChecked ? 1.0f : 0.0f;
        if (!wasDrawnLastFrame)
            checkmarkStrokeBlend = checkmarkTargetBlend;
        else
            checkmarkStrokeBlend += (checkmarkTargetBlend - checkmarkStrokeBlend) * checkmarkSmoothingStep;

        if (checkmarkStrokeBlend > 0.001f) {
            const float checkboxSize = checkboxRect.GetWidth();
            const float markPadding = ImMax(1.0f, IM_FLOOR(checkboxSize / 6.0f));
            const float markSize = checkboxSize - markPadding * 2.0f;
            RenderCheckMarkProgress(window->DrawList, checkboxRect.Min + ImVec2(markPadding, markPadding), checkColor, markSize, checkmarkStrokeBlend);
        }
    }

	bool Checkbox(const char* label, Option* v) {
        ImGui::BeginDisabled(v->IsUnsupportedGameVer());

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) {
            ImGui::EndDisabled();
            return false;
        }

        ImGuiContext& imguiContext = *GImGui;
        const ImGuiStyle& style = imguiContext.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        const float square_sz = GetFrameHeight();
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id)) {
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, imguiContext.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (v->GetValue() ? ImGuiItemStatusFlags_Checked : 0));
            ImGui::EndDisabled();
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
        const ImVec4 cFrame = BuildCheckboxFrameColor(id, hovered, held, imguiContext);
        RenderFrame(check_bb.Min, check_bb.Max, ColorConvertFloat4ToU32(cFrame), true, style.FrameRounding);

        const ImU32 check_col_base = GetColorU32(ImGuiCol_CheckMark);
        bool mixed_value = (imguiContext.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
        if (mixed_value) {
            // Undocumented tristate/mixed/indeterminate checkbox (#2644)
            // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
            ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col_base, style.FrameRounding);
        } else
            DrawCheckboxMarkAnimated(window, check_bb, id, v->GetValue(), imguiContext);

        ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
        if (imguiContext.LogEnabled)
            LogRenderedText(&label_pos, mixed_value ? "[~]" : v->GetValue() ? "[x]" : "[ ]");
        if (label_size.x > 0.0f)
            RenderText(label_pos, label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, imguiContext.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (v->GetValue() ? ImGuiItemStatusFlags_Checked : 0));
        ImGui::EndDisabled();
        return pressed;
	}
    bool Checkbox(const char* label, Option* v, const char* tooltip) {
        bool checkbox = Checkbox(label, v);

        std::string finalTooltip = v->IsUnsupportedGameVer() ? "Option is disabled because it does not support v" + EGSDK::GamePH::GameVerToStr(v->IsUnsupportedGameVer()) : tooltip ? tooltip : "";
        if (!finalTooltip.empty())
            SetItemTooltipAnimated("%s", finalTooltip.c_str());

        return checkbox;
    }
    bool CheckboxHotkey(const char* label, KeyBindOption* v, const char* tooltip) {
        bool checkbox = Checkbox(label, v, tooltip);
        Hotkey(std::string(label + std::string("##ToggleKey")), v);
        return checkbox;
    }
    bool SliderInt(const char* label, const char* tooltip, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
        return SliderIntStacked(label, v, v_min, v_max, format, flags, tooltip);
    }
    bool SliderFloat(const char* label, const char* tooltip, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        return SliderFloatStacked(label, v, v_min, v_max, format, flags, tooltip);
    }
    static const float CalculateIndentation(const float window_width, const float text_width, const float min_indentation) {
        const float indentation = (window_width - text_width) * 0.5f;
        return (indentation > min_indentation ? indentation : min_indentation);
    }
    void TextCenteredV(const char* fmt, va_list args) {
        const float min_indentation = 20.0f;
        const float window_width = GetWindowSize().x - GImGui->Style.ScrollbarSize - (GImGui->Style.WindowPadding.x);
        const float wrap_pos = window_width - min_indentation;

        const char* text{};
        const char* text_end{};
        ImFormatStringToTempBufferV(&text, &text_end, fmt, args);

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
            TextV(line.c_str(), args);
            NewLine();
            line = word;
        }

        if (!line.empty()) {
            SameLine(CalculateIndentation(window_width, CalcTextSize(line.c_str()).x, min_indentation));
            TextV(line.c_str(), args);
            NewLine();
        }
    }
    void TextCentered(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        TextCenteredV(fmt, args);
        va_end(args);
    }
    void TextCenteredColoredV(const char* fmt, ImU32 col, va_list args) {
        PushStyleColor(ImGuiCol_Text, col);
        TextCenteredV(fmt, args);
        PopStyleColor();
    }
    void TextCenteredColored(const char* fmt, ImU32 col, ...) {
        va_list args;
        va_start(args, col);
        TextCenteredColoredV(fmt, col, args);
        va_end(args);
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
        return ButtonSmooth(label, size);
        NewLine();
    }
    void SeparatorTextColored(const char* label, ImU32 col) {
        PushStyleColor(ImGuiCol_Text, col);
        SeparatorText(label);
        PopStyleColor();
    }
    void SeparatorTextSection(const char* label, bool leadingGap) {
        if (leadingGap)
            Dummy(ImVec2(0.0f, GetStyle().ItemSpacing.y * 0.45f));
        const ImGuiStyle& st = GetStyle();
        const ImVec4& base = st.Colors[ImGuiCol_Text];
        const ImVec4& pop = st.Colors[ImGuiCol_SliderGrab];
        // Strong lean on accent: default Text is cool (high B); a shallow lerp reads magenta with rose accents.
        constexpr float accentStrength = 0.82f;
        const ImVec4 mix(ImLerp(base.x, pop.x, accentStrength), ImLerp(base.y, pop.y, accentStrength), ImLerp(base.z, pop.z, accentStrength), 1.0f);
        PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(mix));
        SeparatorText(label);
        PopStyleColor();
    }
    void SeparatorTextSectionColored(const char* label, ImU32 col, bool leadingGap) {
        if (leadingGap)
            Dummy(ImVec2(0.0f, GetStyle().ItemSpacing.y * 0.45f));
        SeparatorTextColored(label, col);
    }
    static void DisplaySimplePopupMessageBase(const char* popupTitle, const char* fmt, va_list args) {
        if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextV(fmt, args);

            if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    static void DisplaySimplePopupMessageBase(float itemWidth, float scale, const char* popupTitle, const char* fmt, va_list args) {
        if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SetNextItemWidth(itemWidth * scale);
            ImGui::TextCenteredV(fmt, args);

            ImGui::SetNextItemWidth(itemWidth * scale);
            if (ImGui::Button("OK", ImVec2(itemWidth, 0.0f) * scale))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    void DisplaySimplePopupMessage(const char* popupTitle, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        DisplaySimplePopupMessageBase(popupTitle, fmt, args);
        va_end(args);
    }
    void DisplaySimplePopupMessageCentered(const char* popupTitle, const char* fmt, ...) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
        va_list args;
        va_start(args, fmt);
        DisplaySimplePopupMessageBase(popupTitle, fmt, args);
        va_end(args);
    }
    void DisplaySimplePopupMessage(float itemWidth, float scale, const char* popupTitle, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        DisplaySimplePopupMessageBase(itemWidth, scale, popupTitle, fmt, args);
        va_end(args);
    }
    void DisplaySimplePopupMessageCentered(float itemWidth, float scale, const char* popupTitle, const char* fmt, ...) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
        va_list args;
        va_start(args, fmt);
        DisplaySimplePopupMessageBase(itemWidth, scale, popupTitle, fmt, args);
        va_end(args);
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