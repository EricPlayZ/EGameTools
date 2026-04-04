#pragma once
#include <EGSDK\ClassHelpers.h>
#include <EGSDK\Engine\IPhysics.h>
#include <EGSDK\Exports.h>

namespace EGSDK::Engine {
	class CBulletPhysicsCharacter;

	// Mirrors engine: IPhysics* CoPhysics::GetPhysics() const  (mangled UEBA = const).
	class EGameSDK_API CoPhysics {
	public:
		DynamicField(CoPhysics, CBulletPhysicsCharacter*, pCBulletPhysicsCharacter);

		IPhysics* GetPhysics() const;
	};
}
