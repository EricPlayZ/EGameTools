#include <EGSDK\GamePH\LogicalLevel.h>
#include <EGSDK\GamePH\SessionCooperativeDI.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	static LogicalLevel* GetOffset_LogicalLevel() {
		SessionCooperativeDI* session = SessionCooperativeDI::Get();
		return session ? session->pLogicalLevel : nullptr;
	}

	LogicalLevel* LogicalLevel::Get() {
		return ClassHelpers::SafeGetter<LogicalLevel>(GetOffset_LogicalLevel, false);
	}
}
