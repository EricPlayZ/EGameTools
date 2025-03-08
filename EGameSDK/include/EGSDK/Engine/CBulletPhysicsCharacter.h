#pragma once
#include <EGSDK\vec3.h>
#include <EGSDK\Engine\IPhysicsCharacter.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::Engine {
	class EGameSDK_API CBulletPhysicsCharacter : IPhysicsCharacter {
	public:
		union {
			DynamicField(CBulletPhysicsCharacter, vec3, playerPos);
			DynamicField(CBulletPhysicsCharacter, vec3, playerPos2);
			DynamicField(CBulletPhysicsCharacter, float, playerDownwardVelocity);
		};

		static vec3 posBeforeFreeze;

		void FreezeCharacter();
		void MoveCharacter(const vec3& pos);

		static CBulletPhysicsCharacter* Get();
	};
}