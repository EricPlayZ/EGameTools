#include <pch.h>
#include "..\game\GamePH\Other.h"
#include "menu.h"

namespace Menu {
    const std::string title = "EGameTools (" + std::string(MOD_VERSION_STR) + ")";
    ImGuiStyle defStyle{};
    ImTextureID EGTLogoTexture{};
    static constexpr ImVec2 defEGTLogoSize = ImVec2(278.0f, 100.0f);
    static ImVec2 EGTLogoSize = defEGTLogoSize;

	static constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
    static constexpr ImVec2 defMinWndSize = ImVec2(425.0f, 725.0f);
    static ImVec2 minWndSize = defMinWndSize;
    static constexpr ImVec2 defMaxWndSize = ImVec2(900.0f, 725.0f);
    static ImVec2 maxWndSize = defMaxWndSize;

    KeyBindOption menuToggle = KeyBindOption(VK_F5);
    float opacity = 99.0f;
    float scale = 1.0f;

    Option firstTimeRunning{};
    Option hasSeenChangelog{};

	void Render() {
        ImGui::StyleScaleAllSizes(&ImGui::GetStyle(), scale, &defStyle);
        ImGui::GetIO().FontGlobalScale = scale;
        minWndSize = defMinWndSize * scale;
        maxWndSize = defMaxWndSize * scale;
        EGTLogoSize = defEGTLogoSize * scale;

        ImGui::SetNextWindowBgAlpha(static_cast<float>(opacity) / 100.0f);
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        ImGui::Begin(title.c_str(), &menuToggle.value, windowFlags); {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2.0f) - (EGTLogoSize.x / 2.0f));
            ImGui::Image(EGTLogoTexture, EGTLogoSize);

            const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.0f + (Core::gameVer != GAME_VER_COMPAT ? (ImGui::CalcTextSize("").y * 2.0f) + GImGui->Style.FramePadding.y + GImGui->Style.ItemSpacing.y : 0.0f) + GImGui->Style.WindowPadding.y * 2.0f + GImGui->Style.FramePadding.y * 2.0f;
            const float remainingHeight = ImGui::GetContentRegionAvail().y - footerHeight;

            if (ImGui::BeginTabBar("##MainTabBar")) {
                for (auto& tab : *MenuTab::GetInstances()) {
                    static float childWidth = 0.0f;

                    ImGui::SpanNextTabAcrossWidth(childWidth, MenuTab::GetInstances()->size());
                    if (ImGui::BeginTabItem(tab.second->tabName.data())) {
                        ImGui::SetNextWindowBgAlpha(static_cast<float>(opacity) / 100.0f);
                        ImGui::SetNextWindowSizeConstraints(ImVec2(std::fmax(minWndSize.x - GImGui->Style.WindowPadding.x * 2.0f, ImGui::CalcTextSize(Core::gameVer > GAME_VER_COMPAT ? "Please wait for a new mod update." : "Upgrade your game version to one that the mod supports.").x), remainingHeight), ImVec2(maxWndSize.x - GImGui->Style.WindowPadding.x * 2.0f, remainingHeight));
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
            ImGui::SliderFloat("Menu Scale", &scale, 1.0f, 2.5f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp);
            if (Core::gameVer != GAME_VER_COMPAT) {
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "Incompatible game version detected!");
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(IM_COL32(200, 0, 0, 255)), "Compatible game version: v%s", GamePH::GameVerToStr(GAME_VER_COMPAT));
            }
            ImGui::End();
        }
	}
}