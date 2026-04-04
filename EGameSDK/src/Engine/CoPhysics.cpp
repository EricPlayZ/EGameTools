#include <EGSDK\Engine\CoPhysics.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::Engine {
	IPhysics* CoPhysics::GetPhysics() const {
		return Utils::Memory::SafeCallFunction<IPhysics*>(
			"engine_x64_rwdi.dll", "?GetPhysics@CoPhysics@@UEBAPEAVIPhysics@@XZ", nullptr, this);
	}
}
