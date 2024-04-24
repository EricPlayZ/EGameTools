	#pragma once
#include "..\buffer.h"

namespace GamePH {
	class gen_TPPModel {
	public:
		union {
			buffer<0x2DB9, bool> enableTPPModel1;
			buffer<0x2DBA, bool> enableTPPModel2;
		};

		static gen_TPPModel* Get();
	};
}