#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <ImGui\imgui_hotkey.h>
#include <ImGui\imgui_internal.h>
#include <ImGui\imguiex.h>
#include <EGSDK\Core\Core.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGSDK\GamePH\GamePH_Hooks.h>
#include <EGT\Menu\Init.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\MenuIcons.h>
#include <EGT\Core\Core.h>

namespace EGT::Menu {
    const std::string title = "EGameTools (" + std::string(MOD_VERSION_STR) + ")";
    ImGuiStyle defStyle{};
    ImTextureID EGTLogoTexture{};
    static constexpr ImVec2 defEGTLogoSize = ImVec2(220.0f, 79.0f);
    static ImVec2 EGTLogoSize = defEGTLogoSize;

    static constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    static constexpr ImVec2 defMinWndSize = ImVec2(700.0f, 640.0f);
    static ImVec2 minWndSize = defMinWndSize;
    static constexpr ImVec2 defMaxWndSize = ImVec2(1180.0f, 920.0f);
    static ImVec2 maxWndSize = defMaxWndSize;
    static constexpr ImVec2 defDefaultWndSize = ImVec2(820.0f, 720.0f);
    static constexpr float defSidebarWidth = 220.0f;

    ImGui::KeyBindOption menuToggle{ false, VK_F5 };
    float opacity = 86.0f;
    float childPanelAlpha = 72.0f;
    float frameAlpha = 90.0f;
    float popupAlpha = 88.0f;
    float micaWashStrength = 55.0f;
    float micaBlurStrength = 90.0f;
    float scale = 1.0f;

    static float lastScaleForMainWindow = -1.0f;
    /// Draft for the menu scale slider; `scale` commits on release so fonts/layout do not thrash each frame while dragging.
    static float menuScaleDraft = 1.0f;
    /// Avoid `SetNextWindowSize(..., Always)` while dragging menu scale — resizing reflows the window and fights the slider under the cursor.
    static bool menuScaleSliderDragThisFrame = false;
    static bool menuScaleSliderDragLastFrame = false;

    ImGui::Option firstTimeRunning{ true };
    ImGui::Option hasSeenChangelog{ false };

    int currentTabIndex = 0;

    /// Wall-clock durations (seconds). Progress uses `deltaSeconds / duration` so frame rate does not change total time.
    static constexpr float menuOpenDurationSec = 0.28f;
    static constexpr float tabCrossfadeDurationSec = 0.34f;
    static constexpr float tabCrossSlideInPx = 12.0f;
    static constexpr int settingsNavId = 999001;

    static float menuOpenLinear = 0.0f;
    static bool menuWasOpenLastPoll = false;

    static bool backdropRectValid = false;
    static float backdropMinX = 0.0f;
    static float backdropMinY = 0.0f;
    static float backdropMaxX = 0.0f;
    static float backdropMaxY = 0.0f;

    /// Tab crossfade: A = outgoing panel, B = incoming; t in [0,1]. Stable index updates when t reaches 1.
    static int tabCrossFromIndex = 0;
    static int tabCrossToIndex = 0;
    static float tabCrossProgress = 1.0f;
    static int tabStableIndex = std::numeric_limits<int>::min();
    static bool tabScrollNeedsReset = false;
    static bool tabWasCrossfadingLastFrame = false;

    /// Smooth vertical scroll for a scrolling child (`NoScrollWithMouse` so Dear ImGui leaves `MouseWheel` for us until EndFrame).
    static void ApplySmoothScrollAfterTabChildContent(float deltaSeconds, bool scrollWasReset, bool clearAllTargets) {
        static std::unordered_map<ImGuiID, float> targetScrollByWindowId;
        if (clearAllTargets)
            targetScrollByWindowId.clear();

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (!w || w->ScrollMax.y <= 0.0f)
            return;

        const ImGuiID wid = w->ID;
        const float cur = ImGui::GetScrollY();
        const float maxScroll = ImGui::GetScrollMaxY();

        auto it = targetScrollByWindowId.find(wid);
        if (scrollWasReset || it == targetScrollByWindowId.end()) {
            targetScrollByWindowId[wid] = cur;
            it = targetScrollByWindowId.find(wid);
        }
        float& tgt = it->second;
        tgt = ImClamp(tgt, 0.0f, maxScroll);

        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            const float max_step = w->InnerRect.GetHeight() * 0.67f;
            const float scroll_step = ImTrunc(ImMin(5.0f * w->CalcFontSize(), max_step));
            tgt -= io.MouseWheel * scroll_step;
            tgt = ImClamp(tgt, 0.0f, maxScroll);
        }

        const float smooth = 1.0f - std::exp(-16.0f * deltaSeconds);
        ImGui::SetScrollY(cur + (tgt - cur) * smooth);
    }

    bool MenuAnimNeedsFrame() {
        return menuToggle.value || menuOpenLinear > 0.0001f;
    }

    bool GetMenuBackdropRectPx(float* outMinX, float* outMinY, float* outMaxX, float* outMaxY) {
        if (!backdropRectValid || !outMinX || !outMinY || !outMaxX || !outMaxY)
            return false;
        *outMinX = backdropMinX;
        *outMinY = backdropMinY;
        *outMaxX = backdropMaxX;
        *outMaxY = backdropMaxY;
        return true;
    }

    float GetMicaBackdropFade() {
        return menuOpenLinear * (micaBlurStrength / 100.0f);
    }

    void UpdateMenuVisibilityAnimFromPoll() {
        const bool open = menuToggle.value;
        if (open && !menuWasOpenLastPoll) {
            menuOpenLinear = 0.0f;
            tabStableIndex = std::numeric_limits<int>::min();
            tabCrossProgress = 1.0f;
        }
        menuWasOpenLastPoll = open;
    }

    static ImVec4 LerpColor4(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(ImLerp(a.x, b.x, t), ImLerp(a.y, b.y, t), ImLerp(a.z, b.z, t), ImLerp(a.w, b.w, t));
    }

    static void ApplyMenuColorAlphas(ImGuiStyle& menuStyle) {
        const ImGuiStyle& defaultStyle = defStyle;
        const float childAlphaNormalized = ImClamp(childPanelAlpha * 0.01f, 0.0f, 1.0f);
        menuStyle.Colors[ImGuiCol_ChildBg] = ImVec4(
            defaultStyle.Colors[ImGuiCol_ChildBg].x,
            defaultStyle.Colors[ImGuiCol_ChildBg].y,
            defaultStyle.Colors[ImGuiCol_ChildBg].z,
            childAlphaNormalized);

        const float frameAlphaNormalized = ImClamp(frameAlpha * 0.01f, 0.0f, 1.0f);
        const float defaultFrameBgAlpha = defaultStyle.Colors[ImGuiCol_FrameBg].w;
        menuStyle.Colors[ImGuiCol_FrameBg] = defaultStyle.Colors[ImGuiCol_FrameBg];
        menuStyle.Colors[ImGuiCol_FrameBg].w = frameAlphaNormalized;
        menuStyle.Colors[ImGuiCol_FrameBgHovered] = defaultStyle.Colors[ImGuiCol_FrameBgHovered];
        menuStyle.Colors[ImGuiCol_FrameBgActive] = defaultStyle.Colors[ImGuiCol_FrameBgActive];
        if (defaultFrameBgAlpha > 1e-5f) {
            menuStyle.Colors[ImGuiCol_FrameBgHovered].w =
                ImMin(1.0f, frameAlphaNormalized * (defaultStyle.Colors[ImGuiCol_FrameBgHovered].w / defaultFrameBgAlpha));
            menuStyle.Colors[ImGuiCol_FrameBgActive].w =
                ImMin(1.0f, frameAlphaNormalized * (defaultStyle.Colors[ImGuiCol_FrameBgActive].w / defaultFrameBgAlpha));
        }

        const float popupAlphaNormalized = ImClamp(popupAlpha * 0.01f, 0.0f, 1.0f);
        menuStyle.Colors[ImGuiCol_PopupBg] = defaultStyle.Colors[ImGuiCol_PopupBg];
        menuStyle.Colors[ImGuiCol_PopupBg].w = popupAlphaNormalized;
    }

    void InitMenuScaleDraftToCommitted() {
        menuScaleDraft = scale;
    }

    void PushScaledMenuStyles() {
        ImGui::StyleScaleAllSizes(&ImGui::GetStyle(), scale, &defStyle);
        ApplyMenuColorAlphas(ImGui::GetStyle());
        ImGui::GetIO().FontGlobalScale = 1.0f;
    }

    /// In-window depth wash (cool neutral). Backdrop blur comes from the D3D12 Mica pass, not from these rects.
    static void DrawMenuMicaBase(ImDrawList* dl, const ImRect& work, const ImGuiStyle& style, float menuOpenFade) {
        if (!dl || work.GetWidth() < 2.0f || work.GetHeight() < 2.0f)
            return;
        const ImVec4& wb = style.Colors[ImGuiCol_WindowBg];
        const float a = 0.070f * menuOpenFade;
        // Neutral cool-gray depth only (red stays on accents in the style sheet).
        const ImVec4 tl(ImLerp(wb.x, 0.12f, 0.52f), ImLerp(wb.y, 0.11f, 0.48f), ImLerp(wb.z, 0.168f, 0.62f), a);
        const ImVec4 tr(ImLerp(wb.x, 0.088f, 0.42f), ImLerp(wb.y, 0.09f, 0.45f), ImLerp(wb.z, 0.14f, 0.55f), a * 0.94f);
        const ImVec4 br(ImLerp(wb.x, 0.058f, 0.42f), ImLerp(wb.y, 0.048f, 0.38f), ImLerp(wb.z, 0.062f, 0.48f), a * 1.06f);
        const ImVec4 bl(ImLerp(wb.x, 0.045f, 0.55f), ImLerp(wb.y, 0.044f, 0.52f), ImLerp(wb.z, 0.058f, 0.58f), a * 1.02f);
        dl->AddRectFilledMultiColor(work.Min, work.Max, ImGui::ColorConvertFloat4ToU32(tl), ImGui::ColorConvertFloat4ToU32(tr), ImGui::ColorConvertFloat4ToU32(br),
            ImGui::ColorConvertFloat4ToU32(bl));
    }

    /// Outer frame + soft lift drawn on the viewport foreground list so left/right edges are not clipped by the window draw list (ImGui clips vertical strokes at WorkRect).
    static void DrawMenuOuterChrome(ImGuiWindow* window, float menuOpenFade, const ImGuiStyle& style, float scaleUi) {
        if (!window || menuOpenFade < 0.02f)
            return;
        ImGuiViewport* viewport = window->Viewport;
        if (!viewport)
            viewport = ImGui::GetMainViewport();
        ImDrawList* foregroundDrawList = ImGui::GetForegroundDrawList(viewport);
        if (!foregroundDrawList)
            return;

        const ImVec2 cornerMin = window->Pos;
        const ImVec2 cornerMax(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y);
        const ImRect outer(cornerMin, cornerMax);
        if (outer.GetWidth() < 8.0f || outer.GetHeight() < 8.0f)
            return;

        const float rounding = window->WindowRounding;
        // Structural chrome only: cool neutral lift (accent orange is for sliders / CTAs in the style sheet).
        ImVec4 glowRgb = LerpColor4(style.Colors[ImGuiCol_Border], ImVec4(0.52f, 0.56f, 0.68f, 1.0f), 0.40f);
        constexpr int glowRingCount = 5;
        const float glowReach = ImMax(5.0f * scaleUi, 3.5f);
        const float peakGlowAlpha = 0.038f * menuOpenFade;
        for (int ringIndex = 0; ringIndex < glowRingCount; ++ringIndex) {
            // Keep expansion > 0 so we never full-screen fill the menu (was 0 on last ring with / (n-1)).
            const float expansion = glowReach * (float)(glowRingCount - ringIndex) / (float)glowRingCount;
            const float normalized = glowReach > 1e-4f ? expansion / glowReach : 0.0f;
            const float envelope = std::exp(-3.4f * normalized * normalized);
            const float ringAlpha = peakGlowAlpha * envelope;
            if (ringAlpha < 0.002f)
                continue;
            const ImRect expanded(outer.Min.x - expansion, outer.Min.y - expansion, outer.Max.x + expansion, outer.Max.y + expansion);
            const float expandedRounding = ImMin(rounding + expansion * 0.40f, expanded.GetHeight() * 0.5f);
            foregroundDrawList->AddRectFilled(expanded.Min, expanded.Max, ImGui::ColorConvertFloat4ToU32(ImVec4(glowRgb.x, glowRgb.y, glowRgb.z, ringAlpha)), expandedRounding);
        }

        const float hairlineInset = 0.5f;
        const ImRect hairlineRect(ImVec2(cornerMin.x + hairlineInset, cornerMin.y + hairlineInset), ImVec2(cornerMax.x - hairlineInset, cornerMax.y - hairlineInset));
        ImVec4 edge = LerpColor4(style.Colors[ImGuiCol_Border], ImVec4(0.86f, 0.88f, 0.96f, 1.0f), 0.52f);
        edge.w = 0.48f * menuOpenFade;
        const float hairlineRounding = ImMax(0.0f, rounding - hairlineInset);
        foregroundDrawList->AddRect(hairlineRect.Min, hairlineRect.Max, ImGui::ColorConvertFloat4ToU32(edge), hairlineRounding, 0, 1.5f);
    }

    /// Separates the brand block from nav + content so the logo does not sit inside the same “frame” as the interactive chrome.
    static void DrawMenuBrandDivider(ImDrawList* dl, float contentLeftX, float contentRightX, float dividerY, float menuOpenFade, const ImGuiStyle& style) {
        if (!dl || menuOpenFade < 0.02f || contentRightX <= contentLeftX + 4.0f)
            return;
        ImVec4 line = LerpColor4(style.Colors[ImGuiCol_Separator], style.Colors[ImGuiCol_Border], 0.35f);
        line.w = 0.55f * menuOpenFade;
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(line);
        dl->AddLine(ImVec2(contentLeftX, dividerY), ImVec2(contentRightX, dividerY), col, 1.0f);
    }

    /// Extra depth on child panels — same hue as ChildBg, luminance-only gradient (no accent tint).
    static void DrawMicaChildWash(ImDrawList* dl, const ImRect& inner, const ImGuiStyle& style, float menuOpenFade) {
        if (!dl || inner.GetWidth() < 2.0f || inner.GetHeight() < 2.0f)
            return;
        const ImVec4& cb = style.Colors[ImGuiCol_ChildBg];
        const float a = 0.055f * menuOpenFade;
        const ImVec4 top(ImMin(1.0f, cb.x * 1.04f), ImMin(1.0f, cb.y * 1.04f), ImMin(1.0f, cb.z * 1.06f), a);
        const ImVec4 bot(cb.x * 0.97f, cb.y * 0.97f, cb.z * 0.99f, a * 0.88f);
        dl->AddRectFilledMultiColor(inner.Min, inner.Max, ImGui::ColorConvertFloat4ToU32(top), ImGui::ColorConvertFloat4ToU32(top),
            ImGui::ColorConvertFloat4ToU32(bot), ImGui::ColorConvertFloat4ToU32(bot));
    }

    /// Soft outer shadow outside card bounds only. Never use ex==0 here — that used to full-fill the panel with tint.
    static void DrawElevatedPanelAura(ImDrawList* dl, const ImRect& bb, float rounding, float scaleUi, const ImGuiStyle& style, float fade) {
        if (!dl || fade < 0.02f || bb.GetWidth() < 2.0f)
            return;
        ImVec4 c = LerpColor4(style.Colors[ImGuiCol_Border], ImVec4(0.72f, 0.76f, 0.90f, 1.0f), 0.22f);
        c = LerpColor4(c, style.Colors[ImGuiCol_WindowBg], 0.55f);
        const float maxEx = ImMax(5.5f * scaleUi, 3.5f);
        constexpr int rings = 7;
        const float peakA = 0.022f * fade;
        for (int i = 0; i < rings; ++i) {
            const float ex = maxEx * (float)(rings - i) / (float)rings;
            const float u = maxEx > 1e-4f ? ex / maxEx : 0.0f;
            const float env = std::exp(-2.9f * u * u);
            const float alpha = peakA * env;
            if (alpha < 0.001f)
                continue;
            const ImRect r(bb.Min.x - ex, bb.Min.y - ex, bb.Max.x + ex, bb.Max.y + ex);
            const float rnd = ImMin(rounding + ex * 0.42f, r.GetHeight() * 0.5f);
            dl->AddRectFilled(r.Min, r.Max, ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, alpha)), rnd);
        }
    }

    static void DrawMicaColumnSheen(ImDrawList* dl, float xCenter, float y0, float y1, float menuOpenFade, const ImGuiStyle& style) {
        if (!dl || y1 <= y0 + 1.0f || menuOpenFade < 0.03f)
            return;
        const ImVec4& acc = style.Colors[ImGuiCol_Border];
        const int alpha = (int)(20.0f * menuOpenFade);
        const int alphaHi = (int)(32.0f * menuOpenFade);
        const float half = ImMax(1.0f, 1.25f * scale);
        const ImU32 edge = IM_COL32((int)(acc.x * 255.0f), (int)(acc.y * 255.0f), (int)(acc.z * 255.0f), alpha);
        const ImU32 mid = IM_COL32(235, 238, 248, alphaHi);
        // col_up_left, col_up_right, col_bottom_right, col_bottom_left
        dl->AddRectFilledMultiColor(ImVec2(xCenter - half, y0), ImVec2(xCenter, y1), edge, mid, mid, edge);
        dl->AddRectFilledMultiColor(ImVec2(xCenter, y0), ImVec2(xCenter + half, y1), mid, edge, edge, mid);
    }

    /// Backlight behind nav rows — neutral only (ex>0 always; ex==0 used to paint the whole row orange).
    static void DrawNavPillGlow(ImDrawList* dl, const ImRect& bb, float blend, bool selected, float scaleUi, const ImGuiStyle& style) {
        if (blend < 0.02f || !dl)
            return;
        const ImVec4& bgRail = style.Colors[ImGuiCol_ChildBg];
        ImVec4 halo = LerpColor4(style.Colors[ImGuiCol_Border], ImVec4(0.80f, 0.84f, 0.96f, 1.0f), selected ? 0.28f : 0.14f);
        halo = LerpColor4(halo, bgRail, 0.50f);

        const float strength = blend * (selected ? 0.55f : 0.38f);
        const float rounding = ImMin(style.FrameRounding, bb.GetHeight() * 0.5f);
        const float maxEx = ImMax(5.0f * scaleUi, 3.2f);
        constexpr int rings = 8;
        const float peakA = selected ? 0.014f : 0.011f;

        for (int i = 0; i < rings; ++i) {
            const float ex = maxEx * (float)(rings - i) / (float)rings;
            const float u = maxEx > 1e-4f ? ex / maxEx : 0.0f;
            const float env = std::exp(-2.85f * u * u);
            const float a = ImClamp(peakA * strength * env, 0.0f, 1.0f);
            if (a < 0.001f)
                continue;
            const ImRect r(bb.Min.x - ex, bb.Min.y - ex, bb.Max.x + ex, bb.Max.y + ex);
            const float rnd = ImMin(rounding + ex * 0.48f, r.GetHeight() * 0.5f);
            dl->AddRectFilled(r.Min, r.Max, ImGui::ColorConvertFloat4ToU32(ImVec4(halo.x, halo.y, halo.z, a)), rnd);
        }
    }

    static size_t NavHighlightSlot(int navId) {
        if (navId == settingsNavId)
            return 15u;
        return static_cast<size_t>(std::clamp(navId, 0, 14));
    }

    /// Smooth nav-row fill (mouse hover, keyboard focus, selected). Drawn under text; Selectable uses transparent Header colors.
    static bool NavRowSelectable(int id, const char* iconUtf8, const char* label, bool selected, float rowH, float deltaSeconds, const ImGuiStyle& style) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (!window || window->SkipItems)
            return false;

        const size_t slot = NavHighlightSlot(id);
        static float navBlendBySlot[16]{};
        static bool navWasMouseHoverBySlot[16]{};
        static bool navWasFocusedBySlot[16]{};

        static bool navWasSelectedBySlot[16]{};
        const bool wasSelected = navWasSelectedBySlot[slot];
        navWasSelectedBySlot[slot] = selected;

        const bool wasHot = navWasMouseHoverBySlot[slot] || navWasFocusedBySlot[slot];
        const float target = selected ? 1.0f : (wasHot ? 0.55f : 0.0f);
        // Leaving selection: decay quickly so the old row does not stay “active” through the tab crossfade.
        const float k = selected ? (1.0f - std::exp(-20.0f * deltaSeconds)) : (1.0f - std::exp(-72.0f * deltaSeconds));
        float& blend = navBlendBySlot[slot];
        if (wasSelected && !selected)
            blend *= 0.25f;
        blend += (target - blend) * k;

        ImGui::PushID(id);
        std::string row(iconUtf8);
        row.append("  ");
        row.append(label);

        ImVec2 label_size = ImGui::CalcTextSize(row.c_str(), nullptr, true);
        ImVec2 pos = window->DC.CursorPos;
        pos.y += window->DC.CurrLineTextBaseOffset;

        const float min_x = pos.x;
        const float max_x = window->WorkRect.Max.x;
        // Gap between nav rail inner edge and the rounded row (not extra window padding).
        const float pillInset = IM_TRUNC(style.ItemSpacing.x * 0.55f);
        // Space between row outline and icon/label.
        const float labelPadX = IM_TRUNC(style.FramePadding.x + style.ItemSpacing.x * 0.65f);

        ImRect bb(min_x + pillInset, pos.y, max_x - pillInset, pos.y + rowH);
        {
            const float spacing_y = style.ItemSpacing.y;
            const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
            bb.Min.y -= spacing_U;
            bb.Max.y += (spacing_y - spacing_U);
        }

        // Shrink the pill vertically so adjacent rows show a strip of child bg between highlights.
        const float pillVInset = 4.0f * scale;
        bb.Min.y += pillVInset;
        bb.Max.y -= pillVInset;

        if (blend > 0.001f) {
            DrawNavPillGlow(window->DrawList, bb, blend, selected, scale, style);
            const ImVec4& bg = style.Colors[ImGuiCol_ChildBg];
            const ImVec4 hoverFill = LerpColor4(bg, style.Colors[ImGuiCol_FrameBgHovered], 0.82f);
            const ImVec4 selectedFill = LerpColor4(hoverFill, ImVec4(0.20f, 0.22f, 0.32f, 1.0f), 0.55f);
            const ImVec4 cEnd = selected ? selectedFill : hoverFill;
            const ImVec4 cMix = LerpColor4(bg, cEnd, blend);
            const float rounding = ImMin(style.FrameRounding, bb.GetHeight() * 0.5f);
            window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(cMix), rounding);
            if (selected && blend > 0.45f) {
                ImVec4 outline = style.Colors[ImGuiCol_Border];
                outline.w = 0.58f * blend;
                window->DrawList->AddRect(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(outline), rounding, 0, 1.0f);
            }
        }

        // `min_x` / `max_x` use `DC.CursorPos` + `WorkRect` (absolute). `SetCursorPosX` expects window-local X
        // (see ImGui::SetCursorPosX: CursorPos.x = Pos.x - Scroll.x + local_x).
        const float lineLocalMinX = min_x - window->Pos.x + window->Scroll.x;
        const float contentW = max_x - min_x;
        const float pillW = contentW - 2.0f * pillInset;
        const float selW = ImMax(1.0f, pillW - 2.0f * labelPadX);
        ImGui::SetCursorPosX(lineLocalMinX + pillInset + labelPadX);
        const bool clicked = ImGui::Selectable(row.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(selW, rowH));

        navWasMouseHoverBySlot[slot] = ImGui::IsItemHovered();
        navWasFocusedBySlot[slot] = ImGui::IsItemFocused();
        ImGui::PopID();
        return clicked;
    }

    static MenuTab* FindTabByIndex(int idx) {
        for (const auto& tab : *MenuTab::GetInstances()) {
            if (tab.first == idx)
                return tab.second;
        }
        return nullptr;
    }

    static void EnsureValidTabIndex() {
        if (currentTabIndex == settingsPanelTabIndex)
            return;
        const auto* tabs = MenuTab::GetInstances();
        if (tabs->empty())
            return;
        if (FindTabByIndex(currentTabIndex))
            return;
        currentTabIndex = tabs->begin()->first;
    }

    static const char* SectionIconUtf8(int tabIndex) {
        switch (tabIndex) {
        case 0: return EGT::MenuIcons::iconUser;
        case 1: return EGT::MenuIcons::iconGun;
        case 2: return EGT::MenuIcons::iconCamera;
        case 3: return EGT::MenuIcons::iconLocationArrow;
        case 4: return EGT::MenuIcons::iconEllipsis;
        case 5: return EGT::MenuIcons::iconGlobe;
        case 6: return EGT::MenuIcons::iconBug;
        default: return EGT::MenuIcons::iconEllipsis;
        }
    }

    static void RenderSettingsPanel() {
        ImGui::SeparatorTextSection("Menu", false);
        ImGui::Hotkey("Menu Toggle Key", &menuToggle);
        ImGui::SeparatorTextSection("Appearance");
        ImGui::SliderFloatStacked("Main window opacity", &opacity, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Opacity of the root menu window background.");
        if (EGSDK::Core::rendererAPI == 12) {
            ImGui::SliderFloatStacked("Backdrop blur", &micaBlurStrength, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("GPU frosted-glass blur behind the menu (DirectX 12 only). Independent from panel opacity.");
        } else {
            ImGui::BeginDisabled();
            ImGui::SliderFloatStacked("Backdrop blur", &micaBlurStrength, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Backdrop blur is only available when the game uses DirectX 12.");
            ImGui::TextDisabled("This session is not DirectX 12 — the slider has no effect; only window opacity tints the game behind the menu.");
        }
        ImGui::SliderFloatStacked("Inset / child alpha", &childPanelAlpha, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Opacity of sidebar and tab panel fills. 50% matches the default glass look.");
        ImGui::SliderFloatStacked("Input / frame alpha", &frameAlpha, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Opacity of sliders, combos, and text fields. 70% matches the default theme.");
        ImGui::SliderFloatStacked("Popup alpha", &popupAlpha, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Opacity of popup and tooltip backgrounds.");
        ImGui::SliderFloatStacked("Mica wash strength", &micaWashStrength, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Strength of the decorative gradient wash drawn behind the UI (not the GPU blur).");

        ImGui::SeparatorTextSection("Layout");
        ImGui::PushID("##EGTMenuScale");
        ImGui::SliderFloatStacked("Menu scale", &menuScaleDraft, 1.0f, 2.5f, "%.2fx", ImGuiSliderFlags_AlwaysClamp, nullptr);
        if (ImGui::IsItemDeactivatedAfterEdit())
            scale = menuScaleDraft;
        if (!ImGui::IsItemActive())
            menuScaleDraft = scale;
        if (ImGui::IsItemActive())
            menuScaleSliderDragThisFrame = true;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Release the slider to apply scale (rebuilds fonts and default window size). Prevents layout glitches while dragging.");
        ImGui::PopID();

        ImGui::SeparatorTextSection("Game");
        if (EGSDK::Core::IsGameVerCompatible()) {
            ImGui::TextDisabled("Your game version is supported by this build.");
        } else {
            ImGui::TextColored(ImVec4(0.95f, 0.28f, 0.28f, 1.0f), "Incompatible game version (v%s) detected!",
                EGSDK::GamePH::GameVerToStr(EGSDK::Core::gameVer).c_str());
            ImGui::TextColored(ImVec4(0.95f, 0.28f, 0.28f, 1.0f), "Supported versions: %s",
                EGSDK::Core::GetSupportedGameVersionsStr().c_str());
        }
    }

    static void RenderTabPanelByIndex(int tabIndex) {
        if (tabIndex == settingsPanelTabIndex) {
            RenderSettingsPanel();
            return;
        }
        MenuTab* active = FindTabByIndex(tabIndex);
        ImGui::BeginDisabled(!EGSDK::GamePH::Hooks::didOnPostUpdateHookExecute);
        if (active)
            active->Render();
        ImGui::EndDisabled();
    }

    static void UpdateBackdropRectFromWindow(ImGuiWindow* menuWindow) {
        const ImRect outer(menuWindow->OuterRectClipped.Min, menuWindow->OuterRectClipped.Max);
        backdropRectValid = outer.GetWidth() > 2.0f && outer.GetHeight() > 2.0f;
        if (backdropRectValid) {
            backdropMinX = outer.Min.x;
            backdropMinY = outer.Min.y;
            backdropMaxX = outer.Max.x;
            backdropMaxY = outer.Max.y;
        }
    }

    static void RenderOneTabLayer(int tabIndex, float layerAlpha, float slidePx) {
        const float alphaBeforeTab = ImGui::GetStyle().Alpha;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alphaBeforeTab * layerAlpha);
        ImGui::Dummy(ImVec2(0.0f, slidePx));
        ImGui::PushItemWidth(-1.0f);
        RenderTabPanelByIndex(tabIndex);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
    }

    static void RenderTabContentArea(ImGuiWindow* menuWindow, const ImGuiStyle& style, float deltaSeconds, float micaWashFade, float menuShow, const ImVec2& tabScreenPos, const ImVec2& tabChildSize) {
        if (tabStableIndex == std::numeric_limits<int>::min()) {
            tabStableIndex = currentTabIndex;
            tabCrossFromIndex = tabCrossToIndex = currentTabIndex;
            tabCrossProgress = 1.0f;
        }

        if (currentTabIndex != tabCrossToIndex && tabCrossProgress < 1.0f - 1e-4f) {
            tabCrossFromIndex = (tabCrossProgress < 0.5f) ? tabCrossFromIndex : tabCrossToIndex;
            tabCrossToIndex = currentTabIndex;
            tabCrossProgress = 0.0f;
            tabScrollNeedsReset = true;
        } else if (currentTabIndex != tabStableIndex && tabCrossProgress >= 1.0f - 1e-4f) {
            tabCrossFromIndex = tabStableIndex;
            tabCrossToIndex = currentTabIndex;
            tabCrossProgress = 0.0f;
            tabScrollNeedsReset = true;
        }

        tabCrossProgress = std::fmin(1.0f, tabCrossProgress + deltaSeconds / tabCrossfadeDurationSec);
        if (tabCrossProgress >= 1.0f - 1e-4f) {
            tabStableIndex = currentTabIndex;
            tabCrossFromIndex = tabCrossToIndex = currentTabIndex;
        }

        const float easedProgress = tabCrossProgress * tabCrossProgress * (3.0f - 2.0f * tabCrossProgress);
        const float alphaIn = easedProgress;
        const float slideIn = (1.0f - easedProgress) * tabCrossSlideInPx * scale;
        const bool crossfading = (tabCrossProgress < 1.0f - 1e-4f) && (tabCrossFromIndex != tabCrossToIndex);
        const bool exitingCrossfade = tabWasCrossfadingLastFrame && !crossfading;

        const bool resetTabScroll = tabScrollNeedsReset;
        const bool scrollHardReset = resetTabScroll || exitingCrossfade;
        constexpr ImGuiWindowFlags tabChildWinFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
        constexpr ImGuiWindowFlags tabCrossWinFlags = ImGuiWindowFlags_NoScrollbar;

        if (crossfading) {
            ImGui::SetCursorScreenPos(tabScreenPos);
            ImGui::PushID("tabCross");
            ImGui::BeginChild("##TabCross", tabChildSize, ImGuiChildFlags_Border, tabCrossWinFlags);
            if (ImGuiWindow* tabWindow = ImGui::GetCurrentWindow())
                DrawMicaChildWash(tabWindow->DrawList, ImRect(tabWindow->InnerRect.Min, tabWindow->InnerRect.Max), style, micaWashFade);
            if (resetTabScroll)
                ImGui::SetScrollY(0.0f);
            RenderOneTabLayer(tabCrossToIndex, alphaIn, slideIn);
            ImGui::EndChild();
            ImGui::PopID();
            tabScrollNeedsReset = false;
            const ImRect tabPanelBounds(tabScreenPos, ImVec2(tabScreenPos.x + tabChildSize.x, tabScreenPos.y + tabChildSize.y));
            DrawElevatedPanelAura(menuWindow->DrawList, tabPanelBounds, style.ChildRounding, scale, style, menuShow);
        } else {
            ImGui::SetCursorScreenPos(tabScreenPos);
            ImGui::BeginChild("##TabContent", tabChildSize, ImGuiChildFlags_Border, tabChildWinFlags);
            if (ImGuiWindow* tabWindow = ImGui::GetCurrentWindow())
                DrawMicaChildWash(tabWindow->DrawList, ImRect(tabWindow->InnerRect.Min, tabWindow->InnerRect.Max), style, micaWashFade);
            if (scrollHardReset)
                ImGui::SetScrollY(0.0f);
            tabScrollNeedsReset = false;
            RenderOneTabLayer(currentTabIndex, 1.0f, 0.0f);
            ApplySmoothScrollAfterTabChildContent(deltaSeconds, scrollHardReset, exitingCrossfade);
            ImGui::EndChild();
            DrawElevatedPanelAura(menuWindow->DrawList, ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), style.ChildRounding, scale, style, menuShow);
        }

        tabWasCrossfadingLastFrame = crossfading;
    }

    static void RenderNavRail(ImGuiWindow* menuWindow, const ImGuiStyle& style, float deltaSeconds, float micaWashFade, float menuShow, float navWidth, float navRailY, float bodyHeight, float navRowHeight, float navRowSideGutter, int settingsNavIdValue) {
        ImGui::SetCursorScreenPos(ImVec2(menuWindow->WorkRect.Min.x, navRailY));
        ImGui::BeginChild("##NavRail", ImVec2(navWidth, bodyHeight), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (ImGuiWindow* navWindow = ImGui::GetCurrentWindow())
            DrawMicaChildWash(navWindow->DrawList, ImRect(navWindow->InnerRect.Min, navWindow->InnerRect.Max), style, micaWashFade);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 12.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::Spacing();
        const float sectionLabelX = ImGui::GetCursorPosX() + navRowSideGutter;
        ImGui::SetCursorPosX(sectionLabelX);
        ImGui::TextDisabled("SECTIONS");
        for (const auto& tab : *MenuTab::GetInstances()) {
            const std::string label(tab.second->tabName);
            if (NavRowSelectable(tab.first, SectionIconUtf8(tab.first), label.c_str(), currentTabIndex == tab.first, navRowHeight, deltaSeconds, style))
                currentTabIndex = tab.first;
        }
        const float settingsBlockHeight = navRowHeight + style.ItemSpacing.y * 3.0f + style.SeparatorTextBorderSize;
        const float flexibleNavSpace = ImGui::GetContentRegionAvail().y - settingsBlockHeight;
        if (flexibleNavSpace > 1.0f)
            ImGui::Dummy(ImVec2(0.0f, flexibleNavSpace));
        ImGui::Separator();
        ImGui::Spacing();
        if (NavRowSelectable(settingsNavIdValue, EGT::MenuIcons::iconGear, "Settings", currentTabIndex == settingsPanelTabIndex, navRowHeight, deltaSeconds, style))
            currentTabIndex = settingsPanelTabIndex;
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
        DrawElevatedPanelAura(menuWindow->DrawList, ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), style.ChildRounding, scale, style, menuShow);
    }

	void Render() {
        PushScaledMenuStyles();
        minWndSize = defMinWndSize * scale;
        maxWndSize = defMaxWndSize * scale;
        EGTLogoSize = defEGTLogoSize * scale;
        const ImGuiStyle& menuStyle = ImGui::GetStyle();
        const float deltaSeconds = std::fmin(ImGui::GetIO().DeltaTime, 0.08f);
        const float menuGutter = menuStyle.ItemSpacing.x; // horizontal gap between nav column and tab panel

        EnsureValidTabIndex();

        const bool wantFrame = menuToggle.value || menuOpenLinear > 0.0001f;
        if (!wantFrame) {
            backdropRectValid = false;
            menuScaleSliderDragThisFrame = false;
            menuScaleSliderDragLastFrame = false;
            return;
        }

        menuScaleSliderDragThisFrame = false;

        if (menuToggle.value)
            menuOpenLinear = std::fmin(1.0f, menuOpenLinear + deltaSeconds / menuOpenDurationSec);
        else
            menuOpenLinear = std::fmax(0.0f, menuOpenLinear - deltaSeconds / menuOpenDurationSec);

        const float menuShow = menuOpenLinear;
        const float userBgAlpha = static_cast<float>(opacity) / 100.0f;
        const float openA = userBgAlpha * menuShow;
        ImGui::SetNextWindowBgAlpha(openA);
        const bool deferWinResizeForScaleDrag = menuScaleSliderDragLastFrame;
        if (!deferWinResizeForScaleDrag && std::fabs(scale - lastScaleForMainWindow) > 1.0e-4f) {
            lastScaleForMainWindow = scale;
            ImGui::SetNextWindowSize(defDefaultWndSize * scale, ImGuiCond_Always);
        } else
            ImGui::SetNextWindowSize(defDefaultWndSize * scale, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);

        bool showWindow = menuToggle.value || (menuOpenLinear > 0.0001f);
        ImGui::Begin("EGameTools##MainMenu", &showWindow, windowFlags);
        if (!showWindow)
            menuToggle.value = false;
        {
            const float alphaBeforeMenu = ImGui::GetStyle().Alpha;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alphaBeforeMenu * menuShow);

            ImGuiWindow* const menuWnd = ImGui::GetCurrentWindow();
            UpdateBackdropRectFromWindow(menuWnd);
            const float micaWashFade = menuShow * ImClamp(micaWashStrength * 0.01f, 0.0f, 1.0f);
            DrawMenuMicaBase(menuWnd->DrawList, ImRect(menuWnd->WorkRect.Min, menuWnd->WorkRect.Max), menuStyle, micaWashFade);

            const float navRowH = 36.0f * scale;

            float maxNavLabelW = ImGui::CalcTextSize("SECTIONS").x;
            {
                const std::string settingsRow = std::string(EGT::MenuIcons::iconGear) + "  Settings";
                maxNavLabelW = std::fmax(maxNavLabelW, ImGui::CalcTextSize(settingsRow.c_str()).x);
            }
            for (const auto& tab : *MenuTab::GetInstances()) {
                const std::string label(tab.second->tabName);
                const std::string row = std::string(SectionIconUtf8(tab.first)) + "  " + label;
                maxNavLabelW = std::fmax(maxNavLabelW, ImGui::CalcTextSize(row.c_str()).x);
            }
            // Match NavRowSelectable: pill inset from rail + padding inside pill for icon/label.
            const float navPillInset = IM_TRUNC(menuStyle.ItemSpacing.x * 0.55f);
            const float navLabelPadX = IM_TRUNC(menuStyle.FramePadding.x + menuStyle.ItemSpacing.x * 0.65f);
            const float navRowSideGutter = navPillInset + navLabelPadX;
            const float minNavW = std::fmax(
                std::fmax(defSidebarWidth * scale, maxNavLabelW + navRowSideGutter * 2.0f + menuStyle.WindowPadding.x * 2.0f + menuStyle.ChildBorderSize * 2.0f + 8.0f * scale),
                EGTLogoSize.x + menuStyle.WindowPadding.x * 2.0f + menuStyle.ChildBorderSize * 2.0f + 4.0f * scale);

            // Brand: fixed layout (no “optical centering” math). Same horizontal inset as ##NavRail content; symmetric
            // WindowPadding.y above logo and below version; then menuGutter (column rhythm) before the nav child.
            const ImVec2 workMin = menuWnd->WorkRect.Min;
            const float workBottom = menuWnd->WorkRect.Max.y;
            const float workRight = menuWnd->WorkRect.Max.x;
            const float colLeft = workMin.x;
            const float navW = minNavW;
            const float tabX = colLeft + navW + menuGutter;
            const float tabW = std::fmax(1.0f, workRight - tabX);

            const float railPad = menuStyle.WindowPadding.x + menuStyle.ChildBorderSize;
            const float innerLeft = colLeft + railPad;
            const float innerW = std::fmax(1.0f, navW - 2.0f * railPad);

            ImFont* const verFont = MenuVersionFont();
            const float verFontSize = verFont->FontSize;
            const char* const verStr = MOD_VERSION_STR;
            const ImVec2 verSz = verFont->CalcTextSizeA(verFontSize, FLT_MAX, 0.0f, verStr);

            const float padY = menuStyle.WindowPadding.y;
            const float stackH = EGTLogoSize.y + menuStyle.ItemSpacing.y + verSz.y;
            const float headerBandH = padY + stackH + padY;
            const float navRailY = workMin.y + headerBandH + menuGutter;
            const float bodyH = std::fmax(1.0f, workBottom - navRailY);
            const float tabFullH = workBottom - workMin.y;

            DrawMicaColumnSheen(menuWnd->DrawList, tabX - menuGutter * 0.48f, navRailY, workBottom, menuShow, menuStyle);

            //DrawMenuBrandDivider(menuWnd->DrawList, workMin.x + 1.0f, workRight - 1.0f, navRailY - menuGutter * 0.42f, menuShow, menuStyle);
            DrawMenuOuterChrome(menuWnd, menuShow, menuStyle, scale);

            const float logoY = workMin.y + padY;
            const float logoX = innerLeft + (innerW - EGTLogoSize.x) * 0.5f;
            const float verY = logoY + EGTLogoSize.y + menuStyle.ItemSpacing.y;
            const float verX = innerLeft + (innerW - verSz.x) * 0.5f;

            {
                ImGui::SetCursorScreenPos(workMin);
                ImGui::InvisibleButton("##BrandDrag", ImVec2(navW, headerBandH));
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
                    ImGuiWindow* root = ImGui::GetCurrentWindow()->RootWindow;
                    if (root)
                        ImGui::StartMouseMovingWindow(root);
                }
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddImage(EGTLogoTexture, ImVec2(logoX, logoY), ImVec2(logoX + EGTLogoSize.x - 4.0f, logoY + EGTLogoSize.y));
                dl->AddText(verFont, verFontSize, ImVec2(verX, verY), ImGui::GetColorU32(ImGuiCol_TextDisabled), verStr);
            }

            ImGui::SetCursorScreenPos(ImVec2(tabX, workMin.y));
            const ImVec2 tabScreen0 = ImGui::GetCursorScreenPos();
            const ImVec2 tabChildSize(tabW, tabFullH);
            RenderTabContentArea(menuWnd, menuStyle, deltaSeconds, micaWashFade, menuShow, tabScreen0, tabChildSize);
            RenderNavRail(menuWnd, menuStyle, deltaSeconds, micaWashFade, menuShow, navW, navRailY, bodyH, navRowH, navRowSideGutter, settingsNavId);
            ImGui::PopStyleVar(); // ImGuiStyleVar_Alpha from menu open fade (Push at start of block)

            ImGui::End();
        }

        menuScaleSliderDragLastFrame = menuScaleSliderDragThisFrame;
	}
}
