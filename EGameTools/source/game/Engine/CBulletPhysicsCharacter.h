#pragma once
#include "..\Vector3.h"
#include "..\buffer.h"

namespace Engine {
	class CBulletPhysicsCharacter {
	public:
		union {
			buffer<0x8A0, Vector3> playerPos2;
			buffer<0x8B8, Vector3> playerPos;
			buffer<0xC38, float> playerDownwardVelocity;
		};

		static Vector3 posBeforeFreeze;

		void FreezeCharacter();
		void MoveCharacter(const Vector3& pos);

		static CBulletPhysicsCharacter* Get();
	};
}