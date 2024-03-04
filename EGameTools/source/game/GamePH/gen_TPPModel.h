	#pragma once
#include "..\buffer.h"

namespace GamePH {
	class gen_TPPModel {
	public:
		union {
			buffer<0x2D59, bool> enableTPPModel1;
			buffer<0x2D5A, bool> enableTPPModel2;
		};

		static gen_TPPModel* Get();
	};
}