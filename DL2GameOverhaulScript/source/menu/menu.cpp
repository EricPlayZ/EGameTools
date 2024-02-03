#define IMGUI_DEFINE_MATH_OPERATORS
#include <Hotkey.h>
#include <ImGuiEx.h>
#include "menu.h"

namespace Menu {
	static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar;
    static const ImVec2 minWndSize = ImVec2(0.0f, 0.0f);
    static const ImVec2 defMaxWndSize = ImVec2(900.0f, 675.0f);
    static ImVec2 maxWndSize = defMaxWndSize;

    KeyBindOption menuToggle = KeyBindOption(VK_F5);
    float transparency = 99.0f;
    float scale = 1.0f;

	void Render() {
        ImGuiStyle* style = &ImGui::GetStyle();
        style->Colors[ImGuiCol_WindowBg] = ImVec4(style->Colors[ImGuiCol_WindowBg].x, style->Colors[ImGuiCol_WindowBg].y, style->Colors[ImGuiCol_WindowBg].z, static_cast<float>(transparency) / 100.0f);

        maxWndSize = defMaxWndSize * scale;
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        ImGui::Begin("EGameTools", &menuToggle.value, windowFlags); {
            if (ImGui::BeginTabBar("##MainTabBar")) {
                for (auto& tab : *MenuTab::GetInstances()) {
                    if (ImGui::BeginTabItem(tab.second->tabName.data())) {
                        tab.second->Render();
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();

            ImGui::Hotkey("Menu Toggle Key", menuToggle);
            ImGui::SliderFloat("Menu Transparency", &transparency, 0.0f, 100.0f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::SliderFloat("Menu Scale", &scale, 1.0f, 2.5f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp)) {
                ImGui::StyleScaleAllSizes(style, scale);
                ImGui::GetIO().FontGlobalScale = scale;
            }
            ImGui::End();
        }
	}
}