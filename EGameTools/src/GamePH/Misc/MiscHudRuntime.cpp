#include <EGSDK\GamePH\GameDI_PH.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGT\GamePH\Misc\MiscRuntime_Internal.h>
#include <EGT\Menu\Misc.h>

namespace EGT::GamePH::Misc {
	void UpdateMiscHudAndAfkRuntime() {
		auto* iLevel = EGSDK::GamePH::LevelDI::Get();
		Menu::Misc::disableHUD.SetChangesAreDisabled(!iLevel || !iLevel->IsLoaded());
		if (!iLevel)
			return;

		if (!iLevel->IsLoaded() && Menu::Misc::disableHUD.GetValue()) {
			Menu::Misc::disableHUD.SetBothValues(false);
			iLevel->ShowUIManager(true);
			return;
		}
		if (Menu::Misc::disableHUD.HasChanged()) {
			Menu::Misc::disableHUD.SetPrevValue(Menu::Misc::disableHUD.GetValue());
			iLevel->ShowUIManager(!Menu::Misc::disableHUD.GetValue());
		}

		auto* gameDI_PH = EGSDK::GamePH::GameDI_PH::Get();
		if (gameDI_PH)
			gameDI_PH->blockPauseGameOnPlayerAfk = Menu::Misc::disableGamePauseWhileAFK.GetValue();
	}
}
