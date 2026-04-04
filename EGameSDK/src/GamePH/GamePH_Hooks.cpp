#include <EGSDK\Core\Core.h>
#include <EGSDK\GamePH\GamePH_Hooks.h>
#include <EGSDK\GamePH\GameDI_PH2.h>
#include <EGSDK\Utils\Hook.h>

namespace EGSDK::GamePH {
	namespace Hooks {
#pragma region OnPostUpdate
		bool didOnPostUpdateHookExecute = false;

		Utils::Hook::VTHook<GameDI_PH2*, void(*)(void*), void*> OnPostUpdateHook{ "OnPostUpdate", &GameDI_PH2::Get, [](void* pGameDI_PH2) -> void {
			Core::OnPostUpdate();
			OnPostUpdateHook.ExecuteCallbacksWithOriginal(pGameDI_PH2);

			didOnPostUpdateHookExecute = true;
		} };
#pragma endregion
	}
}
