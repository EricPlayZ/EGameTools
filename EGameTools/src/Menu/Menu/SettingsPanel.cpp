#include <ImGui\imguiex.h>
#include <EGSDK\Core\Core.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\MenuView.h>

namespace EGT::Menu::MenuView {
	void RenderSettingsPanel(float& menuScaleDraft, bool& menuScaleSliderDragThisFrame) {
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
			ImGui::TextDisabled("This session is not DirectX 12 - the slider has no effect; only window opacity tints the game behind the menu.");
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
}
