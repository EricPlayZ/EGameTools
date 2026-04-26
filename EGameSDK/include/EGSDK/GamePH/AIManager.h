#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
    namespace ai {
        class BaseAI;
    }

    class EGameSDK_API AIManager {
    public:
        void BindManagerToBaseAI(ai::BaseAI* pBaseAI);
    };
}
