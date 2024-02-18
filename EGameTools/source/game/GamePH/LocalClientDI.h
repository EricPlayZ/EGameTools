#pragma once
#include "..\buffer.h"

namespace GamePH {
	class gen_TPPModel;

	class LocalClientDI {
	public:
		union {
			buffer<0x90, gen_TPPModel*> pgen_TPPModel;
		};

		static LocalClientDI* Get();
	};
}