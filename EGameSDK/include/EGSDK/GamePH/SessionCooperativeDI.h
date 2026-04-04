#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class LocalClientDI;
	class LogicalLevel;

	class EGameSDK_API SessionCooperativeDI {
	public:
		union {
			DynamicField(SessionCooperativeDI, LocalClientDI*, pLocalClientDI);
			DynamicField(SessionCooperativeDI, LogicalLevel*, pLogicalLevel);
		};

		static SessionCooperativeDI* Get();
	};
}