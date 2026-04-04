#include <EGSDK\Engine\CGame.h>
#include <EGSDK\Engine\CLobbySteam.h>
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Offsets.h>
#include <EGSDK\Utils\Memory.h>

namespace EGSDK::Engine {
    static CGame* GetOffset_CGame() {
        void* globalSlot = OffsetManager::Get_CGameGlobalSlot();
        if (globalSlot && !Utils::Memory::IsBadReadPtr(globalSlot, sizeof(void*))) {
            CGame* fromGlobal = *reinterpret_cast<CGame**>(globalSlot);
            if (fromGlobal && !Utils::Memory::IsBadReadPtr(fromGlobal))
                return fromGlobal;
        }
        CLobbySteam* pCLobbySteam = CLobbySteam::Get();
        return pCLobbySteam ? pCLobbySteam->pCGame : nullptr;
    }
    CGame* CGame::Get() {
        return ClassHelpers::SafeGetter<CGame>(GetOffset_CGame, false, false);
    }
}