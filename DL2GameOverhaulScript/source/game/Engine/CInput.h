#pragma once
#include <basetsd.h>

namespace Engine {
	class CInput {
	public:
		DWORD64 BlockGameInput();
		void UnlockGameInput();

		static CInput* Get();
	};
}