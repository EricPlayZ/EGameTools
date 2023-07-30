#include "..\ImGui\imgui.h"
#include "..\game_classes.h"
#include <algorithm>

namespace Menu {
	namespace Player {
		char searchFilter[64];

		void Render() {
			if (ImGui::CollapsingHeader("Player Variables", ImGuiTreeNodeFlags_None)) {
				ImGui::InputText("##VarsSearch", searchFilter, 32);

				for (auto const& [key, val] : GamePH::PlayerVariables::unorderedPlayerVars) {
					if (val.first == NULL)
						continue;

					std::string lowerSearch = key.data();
					std::string lowerFilter = searchFilter;
					std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower);
					std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), tolower);
					if (lowerSearch.find(std::string(lowerFilter)) == std::string::npos)
						continue;

					if (val.second == "float") {
						float* varAddr = reinterpret_cast<float*>(val.first);
						ImGui::InputFloat(key.data(), &*varAddr);
					} else if (val.second == "bool") {
						bool* varAddr = reinterpret_cast<bool*>(val.first);
						ImGui::Checkbox(key.data(), &*varAddr);
					}
				}
			}
		}
	}
}