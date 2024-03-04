#include <pch.h>
#include "..\GamePH\GameDI_PH.h"
#include "..\GamePH\LevelDI.h"
#include "..\GamePH\Other.h"
#include "..\GamePH\gen_TPPModel.h"
#include "..\core.h"
#include "..\menu\camera.h"
#include "..\offsets.h"
#include "CBaseCamera.h"

namespace Engine {
	namespace Hooks {
#pragma region ReadVideoSettings
		static bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1);
		static Utils::Hook::MHook<LPVOID, bool(*)(LPVOID, LPVOID, bool)> ReadVideoSettingsHook{ "ReadVideoSettings", &Offsets::Get_ReadVideoSettings, &detourReadVideoSettings };

		static bool detourReadVideoSettings(LPVOID instance, LPVOID file, bool flag1) {
			if (Core::rendererAPI)
				return ReadVideoSettingsHook.pOriginal(instance, file, flag1);

			DWORD renderer = *reinterpret_cast<PDWORD>(reinterpret_cast<DWORD64>(instance) + 0x7C);
			Core::rendererAPI = !renderer ? 11 : 12;

			return ReadVideoSettingsHook.pOriginal(instance, file, flag1);
		}
#pragma endregion

#pragma region MoveCameraFromForwardUpPos
		bool switchedFreeCamByGamePause = false;
		Vector3 freeCamPosBeforeGamePause{};

		static void detourMoveCameraFromForwardUpPos(LPVOID pCBaseCamera, float* a3, float* a4, Vector3* pos);
		static Utils::Hook::MHook<LPVOID, void(*)(LPVOID, float*, float*, Vector3*)> MoveCameraFromForwardUpPosHook{ "MoveCameraFromForwardUpPos", &Offsets::Get_MoveCameraFromForwardUpPos, &detourMoveCameraFromForwardUpPos };

		static void detourMoveCameraFromForwardUpPos(LPVOID pCBaseCamera, float* a3, float* a4, Vector3* pos) {
			GamePH::LevelDI* iLevel = GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded() || iLevel->IsTimerFrozen())
				return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

			if (Menu::Camera::freeCam.GetValue() && switchedFreeCamByGamePause) {
				switchedFreeCamByGamePause = false;
				*pos = freeCamPosBeforeGamePause;
				return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);
			}
			if (!Menu::Camera::thirdPersonCamera.GetValue() || Menu::Camera::photoMode.GetValue() || Menu::Camera::freeCam.GetValue() || !pos)
				return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

			CBaseCamera* viewCam = static_cast<CBaseCamera*>(iLevel->GetViewCamera());
			if (!viewCam)
				return MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);

			Vector3 forwardVec{};
			viewCam->GetForwardVector(&forwardVec);
			const Vector3 normForwardVec = forwardVec.normalize();
			Vector3 leftVec{};
			viewCam->GetLeftVector(&leftVec);
			const Vector3 normLeftVec = leftVec.normalize();

			Vector3 newCamPos = *pos - normForwardVec * -Menu::Camera::tpDistanceBehindPlayer;
			newCamPos.Y += Menu::Camera::tpHeightAbovePlayer - 1.5f;
			newCamPos -= normLeftVec * Menu::Camera::tpHorizontalDistanceFromPlayer;

			*pos = newCamPos;

			MoveCameraFromForwardUpPosHook.pOriginal(pCBaseCamera, a3, a4, pos);
		}
#pragma endregion

#pragma region fs::open
		static std::vector<std::string> cachedUserModDirs{};
		static Utils::Time::Timer timeSinceCache{ 0 };
		static void CacheUserModDirs() {
			spdlog::warn("Recaching user mod directories");

			if (!cachedUserModDirs.empty())
				cachedUserModDirs.clear();

			const char* userModFilesPath = "..\\..\\..\\source\\data\\EGameTools\\UserModFiles";

			cachedUserModDirs.push_back(userModFilesPath);
			for (const auto& entry : std::filesystem::recursive_directory_iterator(userModFilesPath)) {
				const std::filesystem::path pathToDir = entry.path();
				if (!std::filesystem::is_directory(pathToDir))
					continue;

				cachedUserModDirs.push_back(pathToDir.string());
			}
		}

		static LPVOID GetFsOpen() {
			return Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?open@fs@@YAPEAUSFsFile@@V?$string_const@D@ttl@@W4TYPE@EFSMode@@W45FFSOpenFlags@@@Z");
		}
		static DWORD64 detourFsOpen(DWORD64 file, DWORD a2, DWORD a3);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, DWORD, DWORD)> FsOpenHook{ "fs::open", &GetFsOpen, &detourFsOpen };

		static DWORD64 detourFsOpen(DWORD64 file, DWORD a2, DWORD a3) {
			const DWORD64 firstByte = (file >> 56) & 0xFF; // get first byte of addr

			const char* filePath = reinterpret_cast<const char*>(file & 0x1FFFFFFFFFFFFFFF); // remove first byte of addr in case it exists
			const std::string fileName = std::filesystem::path(filePath).filename().string();
			if (fileName.empty() || fileName.contains("EGameTools") || fileName.ends_with(".model"))
				return FsOpenHook.pOriginal(file, a2, a3);

			if (timeSinceCache.DidTimePass()) {
				CacheUserModDirs();
				timeSinceCache = Utils::Time::Timer(5000);
			}

			std::string finalPath{};

			try {
				for (const auto& entry : cachedUserModDirs) {
					finalPath = entry + "\\" + fileName;
					if (!std::filesystem::exists(finalPath))
						continue;

					const std::string finalPath2 = std::filesystem::absolute(finalPath).string();
					const char* filePath2 = finalPath2.c_str();
					spdlog::warn("Loading user mod file \"{}\"", finalPath.c_str());

					const DWORD64 finalAddr = firstByte != 0x0 ? (reinterpret_cast<DWORD64>(filePath2) | (firstByte << 56)) : reinterpret_cast<DWORD64>(filePath2); // restores first byte of addr if first byte was not 0

					const DWORD64 result = FsOpenHook.pOriginal(finalAddr, a2, a3);
					if (!result)
						spdlog::error("fs::open returned 0! Something went wrong with loading user mod file \"{}\"!", finalPath.c_str());
					return result;
				}
			} catch (const std::exception& e) {
				spdlog::error("Exception thrown while loading user mod file \"{}\": {}", finalPath.c_str(), e.what());
			}
			return FsOpenHook.pOriginal(file, a2, a3);
		}
#pragma endregion
	}
}