#include <imgui.h>
#include "camera.h"
#include "player.h"
#include "world.h"

namespace Menu {
	static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    static const ImVec2 minWndSize = ImVec2(0.0f, 0.0f);
    static const ImVec2 maxWndSize = ImVec2(900.0f, 675.0f);

	bool isOpen = false;
    float transparency = 99.5f;

	void Render() {
        ImGuiStyle* style = &ImGui::GetStyle();
        style->Colors[ImGuiCol_WindowBg] = ImVec4(style->Colors[ImGuiCol_WindowBg].x, style->Colors[ImGuiCol_WindowBg].y, style->Colors[ImGuiCol_WindowBg].z, static_cast<float>(transparency) / 100.0f);

        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
        ImGui::Begin("EGameTools", &Menu::isOpen, windowFlags); {
            if (ImGui::BeginTabBar("##MainTabBar")) {
                if (ImGui::BeginTabItem("Player")) {
                    Menu::Player::Render();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Camera")) {
                    Menu::Camera::Render();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World")) {
                    Menu::World::Render();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();

            ImGui::SliderFloat("Menu Transparency", &transparency, 0.0f, 100.0f, "%.1f%%", ImGuiSliderFlags_AlwaysClamp);
            ImGui::End();
        }
	}
}