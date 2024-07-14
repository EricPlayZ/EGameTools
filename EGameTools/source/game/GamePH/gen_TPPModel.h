#pragma once
#include "..\buffer.h"

namespace GamePH {
	class gen_TPPModel {
	public:
		union {
			buffer<0x35E1, bool> enableTPPModel1;
			buffer<0x35E2, bool> enableTPPModel2;
		};

		static gen_TPPModel* Get();
	};
}