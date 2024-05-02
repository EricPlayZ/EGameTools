#include <pch.h>

namespace Engine {
	void AuthenticateDataResultsClear(LPVOID instance) {
		void(*pAuthenticateDataResultsClear)(LPVOID instance) = (decltype(pAuthenticateDataResultsClear))Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?Clear@Results@AuthenticateData@@QEAAXXZ");
		if (!pAuthenticateDataResultsClear)
			return;

		pAuthenticateDataResultsClear(instance);
	}
}
