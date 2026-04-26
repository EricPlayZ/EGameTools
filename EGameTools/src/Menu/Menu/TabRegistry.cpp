#include <EGT\Menu\Menu.h>
#include <EGT\Menu\MenuIcons.h>
#include <EGT\Menu\MenuView.h>

namespace EGT::Menu::MenuView {
	MenuTab* FindTabByIndex(int idx) {
		for (const auto& tab : *MenuTab::GetInstances()) {
			if (tab.first == idx)
				return tab.second;
		}
		return nullptr;
	}

	void EnsureValidTabIndex() {
		if (currentTabIndex == settingsPanelTabIndex)
			return;
		const auto* tabs = MenuTab::GetInstances();
		if (tabs->empty())
			return;
		if (FindTabByIndex(currentTabIndex))
			return;
		currentTabIndex = tabs->begin()->first;
	}

	const char* SectionIconUtf8(int tabIndex) {
		switch (tabIndex) {
		case 0: return EGT::MenuIcons::iconUser;
		case 1: return EGT::MenuIcons::iconGun;
		case 2: return EGT::MenuIcons::iconCamera;
		case 3: return EGT::MenuIcons::iconLocationArrow;
		case 4: return EGT::MenuIcons::iconEllipsis;
		case 5: return EGT::MenuIcons::iconGlobe;
		case 6: return EGT::MenuIcons::iconBug;
		default: return EGT::MenuIcons::iconEllipsis;
		}
	}
}
