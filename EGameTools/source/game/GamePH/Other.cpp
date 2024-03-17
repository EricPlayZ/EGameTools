#include <pch.h>
#include "..\offsets.h"
#include "GameDI_PH.h"
#include "gen_TPPModel.h"

namespace GamePH {
	const std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> GetCurrentGameVersion() {
		WCHAR inBuf[MAX_PATH] = { 0 };
		GetModuleFileNameW(GetModuleHandleW(nullptr), inBuf, static_cast<DWORD>(std::size(inBuf)));
		std::wstring fileStr = inBuf;

		DWORD verHandle;
		DWORD verSz = GetFileVersionInfoSizeW(fileStr.data(), &verHandle);

		if (verSz != 0 && verHandle == 0) {

			std::vector<uint8_t> verData(verSz);

			if (GetFileVersionInfoW(fileStr.data(), verHandle, verSz, verData.data())) {

				LPVOID buffer;
				UINT bufferLength;

				if (VerQueryValueW(verData.data(), L"\\", &buffer, &bufferLength) && bufferLength != 0) {

					VS_FIXEDFILEINFO* verInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(buffer);

					if (verInfo->dwSignature == 0xFEEF04BD) {
						const auto major = (verInfo->dwFileVersionMS >> 16) & 0xFFFF;
						const auto minor = verInfo->dwFileVersionMS & 0xFFFF;
						const auto build = (verInfo->dwFileVersionLS >> 16) & 0xFFFF;
						const auto revision = verInfo->dwFileVersionLS & 0xFFFF;
						return { major, minor, build, revision };
					}
				}
			}
		}

		return { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
	}

	const std::string GetCurrentGameVersionStr() {
		auto [major, minor, build, revision] = GetCurrentGameVersion();

		if (major == 0xFFFF || minor == 0xFFFF || build == 0xFFFF || revision == 0xFFFF)
			return "UNKNOWN";

		return std::string(std::to_string(major) + std::to_string(minor) + std::to_string(build) + std::to_string(revision));
	}

	const std::string GameVerToStr(uint16_t version) {
		uint16_t major = version / 10000;
		uint16_t minor = (version / 100) % 100;
		uint16_t build = (version / 10) % 10;
		uint16_t revision = version % 10;

		return std::string(std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build) + "." + std::to_string(revision));
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