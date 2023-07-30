#include "..\ImGui\imgui.h"
#include "camera.h"
#include "player.h"

namespace Menu {
	static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	bool isOpen = false;

	void Render() {
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
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::Separator();
			ImGui::End();
		}
	}
}