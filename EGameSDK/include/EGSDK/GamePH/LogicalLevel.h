#pragma once
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	class LogicalPlayer;

	class EGameSDK_API LogicalLevel {
	public:
		union {
			DynamicField(LogicalLevel, LogicalPlayer*, pLogicalPlayer);
		};

		static LogicalLevel* Get();
	};
}
