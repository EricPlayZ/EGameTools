#include <EGSDK\GamePH\AIManager.h>

namespace EGSDK::GamePH {
	void AIManager::BindManagerToBaseAI(ai::BaseAI* pBaseAI) {
        return Utils::Memory::SafeCallFunctionOffsetVoid(OffsetManager::Get_BindManagerToBaseAI, this, pBaseAI);
    }
}
