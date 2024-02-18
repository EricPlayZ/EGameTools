#pragma once
#include "..\Engine\CBaseCamera.h"

namespace GamePH {
	class TPPCameraDI : public Engine::CBaseCamera {
	public:
		static TPPCameraDI* Get();
	};
}