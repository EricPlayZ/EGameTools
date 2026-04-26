#include <EGSDK\GamePH\CoPlayerRestrictions.h>
#include <EGSDK\GamePH\GamePH_Misc.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGT\GamePH\Player\PlayerRuntime_Internal.h>
#include <EGT\Menu\Player.h>

namespace EGT::GamePH::Player {
	void UpdatePlayerRestrictionsRuntime() {
		if (!Menu::Player::disableSafezoneRestrictions.HasChanged())
			return;
		auto* playerDI_PH = EGSDK::GamePH::PlayerDI_PH::Get();
		if (!playerDI_PH || !playerDI_PH->areRestrictionsEnabledByGame)
			return;
		auto* coPlayerRestrictions = EGSDK::GamePH::CoPlayerRestrictions::Get();
		if (!coPlayerRestrictions)
			return;

		DWORD64 tempFlags = 0;
		DWORD64* flagsPtr = coPlayerRestrictions->GetPlayerRestrictionsFlags(&tempFlags);
		if (flagsPtr)
			coPlayerRestrictions->flags = *flagsPtr;

		Menu::Player::disableSafezoneRestrictions.HasChangedTo(false)
			? playerDI_PH->EnablePlayerRestrictions(&*coPlayerRestrictions->flags.getPointer())
			: playerDI_PH->DisablePlayerRestrictions(&*coPlayerRestrictions->flags.getPointer());

		Menu::Player::disableSafezoneRestrictions.SetPrevValue(Menu::Player::disableSafezoneRestrictions.GetValue());
	}
}
