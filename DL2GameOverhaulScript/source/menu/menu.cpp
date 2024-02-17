#include <pch.h>
#include "menu.h"

namespace Menu {
    const std::string title = "EGameTools (" + std::string(MOD_VERSION_STR) + ")";
    ImGuiStyle defStyle{};
    ImTextureID EGTLogoTexture{};

	static constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
    static constexpr ImVec2 minWndSize = ImVec2(425.0f, 725.0f);
    static constexpr ImVec2 defMaxWndSize = ImVec2(900.0f, 725.0f);
    static ImVec2 maxWndSize = defMaxWndSize;

    KeyBindOption menuToggle = KeyBindOption(VK_F5);
    float opacity = 99.0f;
    float scale = 1.0f;

    Option firstTimeRunning{};
    Option hasSeenChangelog{};

	void Render() {
        maxWndSize = defMaxWndSize * scale;
        ImGui::SetNextWindowBgAlpha(static_cast<float>(opacity) / 100.0f);
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        ImGui::Begin(title.c_str(), &menuToggle.value, windowFlags); {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2.0f) - 278.0f / 2.0f);
            ImGui::Image(EGTLogoTexture, ImVec2(278.0f * scale, 100.0f * scale));

            const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.0f + GImGui->Style.WindowPadding.y * 2.0f + GImGui->Style.FramePadding.y * 2.0f;
            const float remainingHeight = ImGui::GetContentRegionAvail().y - footerHeight;

            if (ImGui::BeginTabBar("##MainTabBar")) {
                for (auto& tab : *MenuTab::GetInstances()) {
                    static float childWidth = 0.0f;

                    ImGui::SpanNextTabAcrossWidth(childWidth, MenuTab::GetInstances()->size());
                    if (ImGui::BeginTabItem(tab.second->tabName.data())) {
                        ImGui::SetNextWindowBgAlpha(static_cast<float>(opacity) / 100.0f);
                        ImGui::SetNextWindowSizeConstraints(ImVec2(minWndSize.x - GImGui->Style.WindowPadding.x * 2.0f, remainingHeight), ImVec2(maxWndSize.x - GImGui->Style.WindowPadding.x * 2.0f, remainingHeight));
                        if (ImGui::BeginChild("##TabChild", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Border)) {
                            childWidth = ImGui::GetItemRectSize().x;
                            tab.second->Render();
                            ImGui::EndChild();
                        }
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBarEx();
            }

            ImGui::Hotkey("Menu Toggle Key", &menuToggle);
            ImGui::SliderFloat("Menu Opacity", &opacity, 0.0f, 100.0f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::SliderFloat("Menu Scale", &scale, 1.0f, 2.5f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp)) {
                ImGui::StyleScaleAllSizes(&ImGui::GetStyle(), scale, &defStyle);
                ImGui::GetIO().FontGlobalScale = scale;
            }
            ImGui::End();
        }
	}
}