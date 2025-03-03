#pragma once
#include <EGSDK\Imports.h>

struct SCommandParam {
public:
    GAME_IMPORT SCommandParam(struct SCommandParam &&);
    GAME_IMPORT SCommandParam(struct SCommandParam const &);
    GAME_IMPORT SCommandParam(enum EVariable, char const *);
    GAME_IMPORT SCommandParam();
    GAME_IMPORT struct SCommandParam & operator=(struct SCommandParam &&);
    GAME_IMPORT struct SCommandParam & operator=(struct SCommandParam const &);
};