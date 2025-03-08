#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	class CGame;

	class EGameSDK_API CLobbySteam {
	public:
		union {
			ClassHelpers::StaticBuffer<0xF8, CGame*> pCGame;
		};

		static CLobbySteam* Get();
	};
}