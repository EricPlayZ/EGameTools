#pragma once
#include "..\buffer.h"

namespace GamePH {
	class TPPCameraDI;

	class CoBaseCameraProxy {
	public:
		union {
			buffer<0xD0, TPPCameraDI*> pTPPCameraDI;
		};
	};
}