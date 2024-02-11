#include <pch.h>
#include "CBulletPhysicsCharacter.h"
#include "CoPhysicsProperty.h"

namespace Engine {
	Vector3 CBulletPhysicsCharacter::posBeforeFreeze{};

	void CBulletPhysicsCharacter::FreezeCharacter() {
		MoveCharacter(posBeforeFreeze);
	}
	void CBulletPhysicsCharacter::MoveCharacter(const Vector3& pos) {
		playerDownwardVelocity = 0.0f;
		playerPos = pos;
		playerPos2 = pos;
	}

	CBulletPhysicsCharacter* CBulletPhysicsCharacter::Get() {
		__try {
			CoPhysicsProperty* pCoPhysicsProperty = CoPhysicsProperty::Get();
			if (!pCoPhysicsProperty)
				return nullptr;

			CBulletPhysicsCharacter* ptr = pCoPhysicsProperty->pCBulletPhysicsCharacter;
			if (!Utils::Memory::IsValidPtrMod(ptr, "engine_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}