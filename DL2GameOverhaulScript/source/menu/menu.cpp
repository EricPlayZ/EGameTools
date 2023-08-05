#include <imgui.h>
#include "camera.h"
#include "player.h"
#include "world.h"

namespace Menu {
	static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    static const ImVec2 minWndSize = ImVec2(0.0f, 0.0f);
    static const ImVec2 maxWndSize = ImVec2(700.0f, 675.0f);

	bool isOpen = false;

	void Render() {
        ImGui::SetNextWindowSizeConstraints(minWndSize, maxWndSize);
		ImGui::Begin("Game Overhaul", &Menu::isOpen, windowFlags); {
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
			ImGui::End();
		}
	}
}