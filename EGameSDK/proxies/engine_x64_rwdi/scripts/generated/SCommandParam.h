#pragma once
#include <EGSDK\Imports.h>

struct SCommandParam {
public:
    GAME_IMPORT SCommandParam(SCommandParam& &);
    GAME_IMPORT SCommandParam(SCommandParam const&);
    GAME_IMPORT SCommandParam(EVariable, char const*);
    GAME_IMPORT SCommandParam();
    GAME_IMPORT ~SCommandParam();
    GAME_IMPORT SCommandParam& operator=(SCommandParam& &);
    GAME_IMPORT SCommandParam& operator=(SCommandParam const&);
};