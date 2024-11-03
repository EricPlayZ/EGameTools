#pragma once
#include "..\Vector3.h"
#include "..\buffer.h"

namespace Engine {
	class CBulletPhysicsCharacter {
	public:
		union {
			buffer<0xA08, Vector3> playerPos2;
			buffer<0xA20, Vector3> playerPos;
			buffer<0xDA0, float> playerDownwardVelocity;
		};

		static Vector3 posBeforeFreeze;

		void FreezeCharacter();
		void MoveCharacter(const Vector3& pos);

		static CBulletPhysicsCharacter* Get();
	};
}