#pragma once
#include <string>
#include <set>
#include <ImGui\imgui.h>
#include <ImGui\imgui_hotkey.h>
#include <EGT\Core\core.h>
#include <EGT\Config\ConfigValue.h>

namespace EGT::Menu {
    /// Sentinel index for the sidebar "Settings" panel (not a registered MenuTab).
    inline constexpr int settingsPanelTabIndex = -1;

    class MenuTab {
    public:
        MenuTab(std::string_view name, int tabIndex) : tabName(name), tabIndex(tabIndex) { GetInstances()->insert({ tabIndex, this}); };
        ~MenuTab() { GetInstances()->erase({ tabIndex, this }); }
        static std::set<std::pair<int, MenuTab*>>* GetInstances() { static std::set<std::pair<int, MenuTab*>> instances{}; return &instances; };

        virtual void Init() {};
        virtual void Render() {};
        virtual void Update() {};

        std::string_view tabName{};
        int tabIndex{};
    };

    extern const std::string title;
    extern ImGuiStyle defStyle;
    extern ImTextureID EGTLogoTexture;

	extern ImGui::KeyBindOption menuToggle;
	/// Panel / ImGui translucency (0–100%).
	extern Config::ConfigFloat opacity;
	/// Child / inset panels (ImGuiCol_ChildBg alpha × 100; 50 ≈ default glass).
	extern Config::ConfigFloat childPanelAlpha;
	/// Sliders, inputs, combo chrome (ImGuiCol_FrameBg* alpha × 100).
	extern Config::ConfigFloat frameAlpha;
	/// Popups / tooltips root (ImGuiCol_PopupBg alpha × 100).
	extern Config::ConfigFloat popupAlpha;
	/// Strength of the decorative Mica gradient wash drawn under ImGui (0–100%).
	extern Config::ConfigFloat micaWashStrength;
	/// DX12 backdrop blur strength (0–100%). Independent from `opacity`.
	extern Config::ConfigFloat micaBlurStrength;
	extern Config::ConfigFloat scale;

    extern ImGui::Option firstTimeRunning;
    extern ImGui::Option hasSeenChangelog;

    extern int currentTabIndex;

    /// Copy committed `scale` into the settings slider draft (call after fonts/config are ready).
    extern void InitMenuScaleDraftToCommitted();

    /// Style scale, per-channel alphas, and font global scale for the current committed `Menu::scale`.
    extern void PushScaledMenuStyles();

    /// Call once per frame before `ImGui::NewFrame()` (even when the menu is closed) so open/close edges are visible to animation state.
    extern void UpdateMenuVisibilityAnimFromPoll();

    /// True while the menu should be drawn (open or mid close fade-out).
    extern bool MenuAnimNeedsFrame();

    /// Screen-space backdrop for DX12 Mica (updated while the main menu is visible). Top-left origin, inclusive min / exclusive max.
    extern bool GetMenuBackdropRectPx(float* outMinX, float* outMinY, float* outMaxX, float* outMaxY);

    /// Menu open fade × (`micaBlurStrength` as 0–1). Drives DX12 frosted composite, not panel alpha.
    extern float GetMicaBackdropFade();

	extern void Render();
}