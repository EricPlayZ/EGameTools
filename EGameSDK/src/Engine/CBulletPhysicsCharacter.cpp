#include <EGSDK\Engine\CBulletPhysicsCharacter.h>
#include <EGSDK\Engine\CoPhysics.h>
#include <EGSDK\GamePH\PlayerDI_PH.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	vec3 CBulletPhysicsCharacter::posBeforeFreeze{};

	void CBulletPhysicsCharacter::FreezeCharacter() {
		MoveCharacter(posBeforeFreeze);
	}
	void CBulletPhysicsCharacter::MoveCharacter(const vec3& pos) {
		playerDownwardVelocity = 0.0f;
		playerPos = pos;
	}

	static CBulletPhysicsCharacter* GetOffset_CBulletPhysicsCharacter() {
		GamePH::PlayerDI_PH* pPlayer = GamePH::PlayerDI_PH::Get();
		if (!pPlayer)
			return nullptr;
		CoPhysics* pCoPhysics = pPlayer->GetCoPhysics();
		if (!pCoPhysics)
			return nullptr;
		return pCoPhysics->pCBulletPhysicsCharacter;
	}
	CBulletPhysicsCharacter* CBulletPhysicsCharacter::Get() {
		return ClassHelpers::SafeGetter<CBulletPhysicsCharacter>(GetOffset_CBulletPhysicsCharacter, false);
	}
}