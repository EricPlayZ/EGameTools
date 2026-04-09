#include <cmath>
#include <cstdarg>
#include <unordered_map>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imgui_internal.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
namespace ImGui {
    static ImGuiStyle defImGuiStyle{};
    static size_t tabIndex = 1;

    struct HoverActiveSmoothState {
        float hover = 0.0f;
        float active = 0.0f;
        unsigned int lastFrame = 0;
        /// Previous frame's raw hover (see `NavRowSelectable` in Menu.cpp). Smoothing targets this so
        /// single-frame hit-test flicker on stacked selectables does not pump the fill.
        bool hoverPrev = false;
    };
    static std::unordered_map<ImGuiID, HoverActiveSmoothState> g_hoverActiveSmoothById;

    enum class HoverCommitMode : unsigned char {
        Default,       // hoverPrev + smooth (headers, sliders)
        ListSelectable // current-frame target; slow in / very fast out (teleport list rows)
    };

    static void SmoothHoverActiveRead(ImGuiID id, float& hoverOut, float& activeOut) {
        const auto it = g_hoverActiveSmoothById.find(id);
        if (it == g_hoverActiveSmoothById.end()) {
            hoverOut = 0.0f;
            activeOut = 0.0f;
            return;
        }
        hoverOut = it->second.hover;
        activeOut = it->second.active;
    }

    static void SmoothHoverActiveCommit(ImGuiID id, bool hovered, bool activeHeld, HoverCommitMode mode = HoverCommitMode::Default) {
        ImGuiContext& g = *GImGui;
        const unsigned int frameCount = static_cast<unsigned int>(g.FrameCount);
        HoverActiveSmoothState& st = g_hoverActiveSmoothById[id];
        const bool continuous = (st.lastFrame + 1u == frameCount);
        st.lastFrame = frameCount;
        const float deltaSeconds = ImMin(g.IO.DeltaTime, 0.08f);
        const float targetActive = activeHeld ? 1.0f : 0.0f;
        if (!continuous) {
            st.hoverPrev = hovered;
            st.hover = hovered ? 1.0f : 0.0f;
            st.active = targetActive;
            return;
        }
        if (mode == HoverCommitMode::ListSelectable) {
            const float targetHover = hovered ? 1.0f : 0.0f;
            // Ease in so hover is visible; ease out much faster than Default so two rows are not both
            // stuck in a dark mid-lerp when moving down the list.
            const float stepHover =
                (targetHover > st.hover) ? (1.0f - std::exp(-14.5f * deltaSeconds)) : (1.0f - std::exp(-95.0f * deltaSeconds));
            st.hover += (targetHover - st.hover) * stepHover;
        } else {
            const float targetHover = st.hoverPrev ? 1.0f : 0.0f;
            // Ease in slowly, ease out fast — avoids header/slider "ghost" when moving between items.
            const float stepHover =
                (targetHover > st.hover) ? (1.0f - std::exp(-22.0f * deltaSeconds)) : (1.0f - std::exp(-56.0f * deltaSeconds));
            st.hover += (targetHover - st.hover) * stepHover;
        }
        st.hoverPrev = hovered;
        const float stepActive =
            (targetActive > st.active) ? (1.0f - std::exp(-24.0f * deltaSeconds)) : (1.0f - std::exp(-58.0f * deltaSeconds));
        st.active += (targetActive - st.active) * stepActive;
    }

    static ImVec4 LerpVec4(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(ImLerp(a.x, b.x, t), ImLerp(a.y, b.y, t), ImLerp(a.z, b.z, t), ImLerp(a.w, b.w, t));
    }

    static void PushSliderSmoothStyleColors(ImGuiID sliderId) {
        float hb = 0.0f;
        float ab = 0.0f;
        SmoothHoverActiveRead(sliderId, hb, ab);
        const ImVec4 f0 = GetStyleColorVec4(ImGuiCol_FrameBg);
        const ImVec4 f1 = GetStyleColorVec4(ImGuiCol_FrameBgHovered);
        const ImVec4 f2 = GetStyleColorVec4(ImGuiCol_FrameBgActive);
        ImVec4 frame = LerpVec4(LerpVec4(f0, f1, hb), f2, ab);
        PushStyleColor(ImGuiCol_FrameBg, frame);
        PushStyleColor(ImGuiCol_FrameBgHovered, frame);
        PushStyleColor(ImGuiCol_FrameBgActive, frame);
        const ImVec4 g0 = GetStyleColorVec4(ImGuiCol_SliderGrab);
        const ImVec4 g1 = GetStyleColorVec4(ImGuiCol_SliderGrabActive);
        const ImVec4 grab = LerpVec4(g0, g1, ab);
        PushStyleColor(ImGuiCol_SliderGrab, grab);
        PushStyleColor(ImGuiCol_SliderGrabActive, grab);
    }

    /// Defined below; used by helpers above.
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
        const ImVec4& textBase = GetStyle().Colors[ImGuiCol_Text];
        const ImVec4 labelEmphasis(ImMin(1.0f, textBase.x * 1.02f), ImMin(1.0f, textBase.y * 1.02f), ImMin(1.0f, textBase.z * 1.01f), textBase.w);
        PushStyleColor(ImGuiCol_Text, labelEmphasis);
        TextUnformatted(label, label_end);
        PopStyleColor();
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
        const ImGuiID sliderId = GetID("##stk");
        PushSliderSmoothStyleColors(sliderId);
        const bool ch = SliderFloat("##stk", v, v_min, v_max, format, flags);
        PopStyleColor(5);
        SmoothHoverActiveCommit(sliderId, IsItemHovered(), IsItemActive());
        if (FindRenderedTextEnd(label) <= label && tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        PopID();
        return ch;
    }

    bool SliderFloat3Stacked(const char* label, float v[3], float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
        PushID(label);
        InlineFormLabel(label, nullptr);
        SetNextItemWidthRemainder();
        const ImGuiID sliderId = GetID("##sf3");
        PushSliderSmoothStyleColors(sliderId);
        const bool ch = SliderFloat3("##sf3", v, v_min, v_max, format, flags);
        PopStyleColor(5);
        SmoothHoverActiveCommit(sliderId, IsItemHovered(), IsItemActive());
        PopID();
        return ch;
    }

    bool SliderIntStacked(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags, const char* tooltip) {
        PushID(label);
        InlineFormLabel(label, tooltip);
        SetNextItemWidthRemainder();
        const ImGuiID sliderId = GetID("##stk");
        PushSliderSmoothStyleColors(sliderId);
        const bool ch = SliderInt("##stk", v, v_min, v_max, format, flags);
        PopStyleColor(5);
        SmoothHoverActiveCommit(sliderId, IsItemHovered(), IsItemActive());
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
        struct ButtonSmoothState {
            float blend = 0.0f;
            bool wasHovered = false;
            bool wasHeld = false;
            unsigned int lastFrame = 0;
        };
        static std::unordered_map<ImGuiID, ButtonSmoothState> buttonSmoothById;
        const unsigned int frameCount = static_cast<unsigned int>(imguiContext.FrameCount);
        ButtonSmoothState& st = buttonSmoothById[smoothId];
        const bool continuous = (st.lastFrame + 1u == frameCount);
        st.lastFrame = frameCount;
        if (!continuous) {
            st.wasHovered = false;
            st.wasHeld = false;
            st.blend = 0.0f;
        }
        const float dt = ImMin(imguiContext.IO.DeltaTime, 0.08f);
        const float smoothingStep = 1.0f - std::exp(-22.0f * dt);
        const bool wasHot = st.wasHovered || st.wasHeld;
        const float target = wasHot ? 1.0f : 0.0f;
        st.blend += (target - st.blend) * smoothingStep;
        const ImVec4 b = GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 h = GetStyleColorVec4(ImGuiCol_ButtonHovered);
        const ImVec4 a = GetStyleColorVec4(ImGuiCol_ButtonActive);
        const float buttonBlend = st.blend;
        const ImVec4 mid(ImLerp(b.x, h.x, buttonBlend), ImLerp(b.y, h.y, buttonBlend), ImLerp(b.z, h.z, buttonBlend), ImLerp(b.w, h.w, buttonBlend));
        PushStyleColor(ImGuiCol_Button, mid);
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImLerp(mid.x, a.x, buttonBlend * 0.4f), ImLerp(mid.y, a.y, buttonBlend * 0.4f), ImLerp(mid.z, a.z, buttonBlend * 0.4f), ImLerp(mid.w, a.w, buttonBlend * 0.4f)));
        PushStyleColor(ImGuiCol_ButtonActive, a);
        const bool pressed = Button(label, size_arg);
        PopStyleColor(3);
        PopID();
        st.wasHovered = IsItemHovered();
        st.wasHeld = IsItemActive();
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

    bool CheckboxAnimated(const char* label, bool* v, const char* tooltip) {
        ImGuiWindow* window = GetCurrentWindow();
        if (!window || window->SkipItems)
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
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
            return false;
        }

        bool hovered, held;
        bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) {
            *v = !(*v);
            MarkItemEdited(id);
        }

        const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
        RenderNavHighlight(total_bb, id);
        const ImVec4 cFrame = BuildCheckboxFrameColor(id, hovered, held, g);
        RenderFrame(check_bb.Min, check_bb.Max, ColorConvertFloat4ToU32(cFrame), true, style.FrameRounding);

        const ImU32 check_col_base = GetColorU32(ImGuiCol_CheckMark);
        bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
        if (mixed_value) {
            ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col_base, style.FrameRounding);
        } else
            DrawCheckboxMarkAnimated(window, check_bb, id, *v, g);

        ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
        if (g.LogEnabled)
            LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
        if (label_size.x > 0.0f)
            RenderText(label_pos, label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
        if (tooltip && *tooltip)
            SetItemTooltipAnimated("%s", tooltip);
        return pressed;
    }

    bool Checkbox(const char* label, bool* v, const char* tooltip) {
        return CheckboxAnimated(label, v, tooltip);
    }

	bool Checkbox(const char* label, Option* v) {
        ImGui::BeginDisabled(v->IsUnsupportedGameVer());

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) {
            ImGui::EndDisabled();
            return false;
        }

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
        const ImVec4 cFrame = BuildCheckboxFrameColor(id, hovered, held, g);
        RenderFrame(check_bb.Min, check_bb.Max, ColorConvertFloat4ToU32(cFrame), true, style.FrameRounding);

        const ImU32 check_col_base = GetColorU32(ImGuiCol_CheckMark);
        bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
        if (mixed_value) {
            // Undocumented tristate/mixed/indeterminate checkbox (#2644)
            // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
            ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col_base, style.FrameRounding);
        } else
            DrawCheckboxMarkAnimated(window, check_bb, id, v->GetValue(), g);

        ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
        if (g.LogEnabled)
            LogRenderedText(&label_pos, mixed_value ? "[~]" : v->GetValue() ? "[x]" : "[ ]");
        if (label_size.x > 0.0f)
            RenderText(label_pos, label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (v->GetValue() ? ImGuiItemStatusFlags_Checked : 0));
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

    bool CollapsingHeaderSmooth(const char* label, ImGuiTreeNodeFlags flags) {
        ImGuiWindow* window = GetCurrentWindow();
        if (!window || window->SkipItems)
            return CollapsingHeader(label, flags);
        const ImGuiID headerId = window->GetID(label);
        float hb = 0.0f;
        float ab = 0.0f;
        SmoothHoverActiveRead(headerId, hb, ab);
        const ImVec4 h0 = GetStyleColorVec4(ImGuiCol_Header);
        const ImVec4 h1 = GetStyleColorVec4(ImGuiCol_HeaderHovered);
        const ImVec4 h2 = GetStyleColorVec4(ImGuiCol_HeaderActive);
        const ImVec4 mix = LerpVec4(LerpVec4(h0, h1, hb), h2, ab);
        PushStyleColor(ImGuiCol_Header, mix);
        PushStyleColor(ImGuiCol_HeaderHovered, mix);
        PushStyleColor(ImGuiCol_HeaderActive, mix);
        const bool open = CollapsingHeader(label, flags);
        PopStyleColor(3);
        SmoothHoverActiveCommit(headerId, IsItemHovered(), IsItemActive());
        return open;
    }

    bool SelectableSmooth(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        const ImGuiID id = window->GetID(label);
        ImVec2 label_size = CalcTextSize(label, NULL, true);
        ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
        ImVec2 pos = window->DC.CursorPos;
        pos.y += window->DC.CurrLineTextBaseOffset;
        ItemSize(size, 0.0f);

        const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
        const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
        const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
        if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
            size.x = ImMax(label_size.x, max_x - min_x);

        const ImVec2 text_min = pos;
        const ImVec2 text_max(min_x + size.x, pos.y + size.y);

        ImRect bb(min_x, pos.y, text_max.x, text_max.y);
        if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0) {
            const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
            const float spacing_y = style.ItemSpacing.y;
            const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
            const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
            bb.Min.x -= spacing_L;
            bb.Min.y -= spacing_U;
            bb.Max.x += (spacing_x - spacing_L);
            bb.Max.y += (spacing_y - spacing_U);
        }

        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        if (span_all_columns) {
            window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
            window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        }

        const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
        const bool item_add = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
        if (span_all_columns) {
            window->ClipRect.Min.x = backup_clip_rect_min_x;
            window->ClipRect.Max.x = backup_clip_rect_max_x;
        }

        if (!item_add)
            return false;

        const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
        if (disabled_item && !disabled_global)
            BeginDisabled();

        if (span_all_columns) {
            if (g.CurrentTable)
                TablePushBackgroundChannel();
            else if (window->DC.CurrentColumns)
                PushColumnsBackground();
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasClipRect;
            g.LastItemData.ClipRect = window->ClipRect;
        }

        ImGuiButtonFlags button_flags = 0;
        if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
        if (flags & ImGuiSelectableFlags_NoSetKeyOwner) { button_flags |= ImGuiButtonFlags_NoSetKeyOwner; }
        if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
        if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
        if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
        if ((flags & ImGuiSelectableFlags_AllowOverlap) || (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap)) { button_flags |= ImGuiButtonFlags_AllowOverlap; }

        const bool was_selected = selected;
        bool hovered, held;
        bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

        if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
            if (g.NavJustMovedToId == id)
                selected = pressed = true;

        if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover))) {
            if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent) {
                SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(window, bb));
                g.NavDisableHighlight = true;
            }
        }
        if (pressed)
            MarkItemEdited(id);

        if (selected != was_selected)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

        float hb = 0.0f;
        float ab = 0.0f;
        SmoothHoverActiveRead(id, hb, ab);
        const ImVec4 rowUnselectedIdle = GetStyleColorVec4(ImGuiCol_FrameBg);
        const ImVec4 fh = GetStyleColorVec4(ImGuiCol_FrameBgHovered);
        const ImVec4 fa = GetStyleColorVec4(ImGuiCol_FrameBgActive);
        ImVec4 rowHoverHi = LerpVec4(fh, fa, 0.72f);
        rowHoverHi.w = ImMax(rowHoverHi.w, 0.94f);
        const ImVec4 rowActiveHi = fa;
        // Selected-at-rest: same family as hover (not Header blend) so it reads on a light list bg.
        ImVec4 rowSelectedIdle = LerpVec4(rowUnselectedIdle, rowHoverHi, 0.66f);
        rowSelectedIdle.w = ImMax(rowSelectedIdle.w, 0.90f);
        const ImVec4 kb = selected ? rowSelectedIdle : rowUnselectedIdle;
        ImVec4 fill = LerpVec4(kb, rowHoverHi, hb);
        fill = LerpVec4(fill, rowActiveHi, ab);
        const float rounding = ImMin(style.FrameRounding, bb.GetHeight() * 0.5f);
        if (selected || hb > 0.02f || ab > 0.02f)
            RenderFrame(bb.Min, bb.Max, ColorConvertFloat4ToU32(fill), false, rounding);
        SmoothHoverActiveCommit(id, hovered, held && hovered, HoverCommitMode::ListSelectable);

        if (g.NavId == id)
            RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_Compact);

        if (span_all_columns) {
            if (g.CurrentTable)
                TablePopBackgroundChannel();
            else if (window->DC.CurrentColumns)
                PopColumnsBackground();
        }

        RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);

        if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup))
            CloseCurrentPopup();

        if (disabled_item && !disabled_global)
            EndDisabled();

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
        return pressed;
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
        // Accent hint without crushing luminance (heavy orange mix made headings look muddy on dark glass).
        constexpr float accentStrength = 0.48f;
        ImVec4 mix(ImLerp(base.x, pop.x, accentStrength), ImLerp(base.y, pop.y, accentStrength), ImLerp(base.z, pop.z, accentStrength), 1.0f);
        mix.x = ImMin(1.0f, mix.x * 1.04f);
        mix.y = ImMin(1.0f, mix.y * 1.03f);
        mix.z = ImMin(1.0f, mix.z * 1.02f);
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