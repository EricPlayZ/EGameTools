#pragma once
#include <EGSDK\Utils\Memory.h>
#define GAME_IMPORT __declspec(dllimport)
#define VIRTUAL_ALIAS(ReturnType, AliasName, Args, CallArgs, TargetName) \
    virtual ReturnType AliasName Args { return TargetName CallArgs; }
#define VIRTUAL_CALL(Index, ReturnType, TargetName, Args, ...) \
    virtual ReturnType TargetName Args { return EGSDK::Utils::Memory::CallVT<Index, ReturnType>(this, ##__VA_ARGS__); }
