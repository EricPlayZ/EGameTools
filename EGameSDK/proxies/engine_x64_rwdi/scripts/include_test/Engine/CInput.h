#pragma once
#include <stdint.h>
#include <EGSDK\Exports.h>

namespace EGSDK::Engine {
	class EGameSDK_API CInput {
	public:
		void UnlockGameInput();
		uint64_t BlockGameInput();

		static CInput* Get();
	};
}