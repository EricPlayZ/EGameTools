#pragma once

#include <EGT\Menu\Menu.h>

namespace EGT::Menu::MenuView {
	extern MenuTab* FindTabByIndex(int idx);
	extern void EnsureValidTabIndex();
	extern const char* SectionIconUtf8(int tabIndex);
	extern void RenderSettingsPanel(float& menuScaleDraft, bool& menuScaleSliderDragThisFrame);
}
