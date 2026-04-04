#include <EGSDK\GamePH\LogicalLevel.h>
#include <EGSDK\GamePH\LogicalPlayer.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	static LogicalPlayer* GetOffset_LogicalPlayer() {
		LogicalLevel* level = LogicalLevel::Get();
		return level ? level->pLogicalPlayer : nullptr;
	}

	LogicalPlayer* LogicalPlayer::Get() {
		return ClassHelpers::SafeGetter<LogicalPlayer>(GetOffset_LogicalPlayer, false);
	}
}