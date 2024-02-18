#include <pch.h>
#include "..\offsets.h"
#include "GameDI_PH.h"
#include "gen_TPPModel.h"

namespace GamePH {
	const DWORD64 GetCurrentGameVersion() {
		DWORD64(*pGetCurrentGameVersion)() = (decltype(pGetCurrentGameVersion))Offsets::Get_GetCurrentGameVersion();
		if (!pGetCurrentGameVersion)
			return 0;

		return pGetCurrentGameVersion();
	}
	const std::string GameVerToStr(DWORD64 version) {
		DWORD64 major = version / 10000;
		DWORD64 minor = (version / 100) % 100;
		DWORD64 patch = version % 100;

		return std::string(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));
	}
	const std::string GetCurrentGameVersionStr() {
		if (!GetCurrentGameVersion())
			return "UNKNOWN";

		DWORD64 version = GetCurrentGameVersion();

		DWORD64 major = version / 10000;
		DWORD64 minor = (version / 100) % 100;
		DWORD64 patch = version % 100;

		return std::string(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));
	}
	
	static DWORD64 ShowTPPModelFunc2(GameDI_PH* pGameDI_PH) {
		DWORD64(*pShowTPPModelFunc2)(LPVOID pGameDI_PH) = (decltype(pShowTPPModelFunc2))Offsets::Get_ShowTPPModelFunc2();
		if (!pShowTPPModelFunc2)
			return 0;

		return pShowTPPModelFunc2(pGameDI_PH);
	}
	void ShowTPPModel(bool showTPPModel) {
		GameDI_PH* pGameDI_PH = GameDI_PH::Get();
		if (!pGameDI_PH)
			return;
		DWORD64 tppFunc2Addr = ShowTPPModelFunc2(pGameDI_PH);
		if (!tppFunc2Addr)
			return;
		void(*pShowTPPModelFunc3)(DWORD64 tppFunc2Addr, bool showTPPModel) = (decltype(pShowTPPModelFunc3))Offsets::Get_ShowTPPModelFunc3();
		if (!pShowTPPModelFunc3)
			return;
		gen_TPPModel* pgen_TPPModel = gen_TPPModel::Get();
		if (!pgen_TPPModel)
			return;

		pShowTPPModelFunc3(tppFunc2Addr, showTPPModel);
	}
	bool ReloadJumps() {
		bool(*pReloadJumps)() = (decltype(pReloadJumps))Offsets::Get_ReloadJumps();
		if (!pReloadJumps)
			return false;

		return pReloadJumps();
	}
}