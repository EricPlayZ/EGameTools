#pragma once
#include <EGSDK\vec3.h>

namespace EGSDK::Engine {
	class EGameSDK_API IControlObject {
	public:
		void SetLocalDir(const vec3* dir);
	};
}