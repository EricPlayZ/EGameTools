#include <pch.h>
#include "..\GamePH\GameDI_PH.h"
#include "..\GamePH\LevelDI.h"
#include "..\GamePH\gameph_misc.h"
#include "..\GamePH\gen_TPPModel.h"
#include "..\core.h"
#include "..\menu\camera.h"
#include "..\menu\misc.h"
#include "..\offsets.h"
#include "CBaseCamera.h"
#include "engine_misc.h"

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
		static const std::string userModFilesFullPath = "\\\\?\\" + std::filesystem::absolute("..\\..\\..\\source\\data\\EGameTools\\UserModFiles").string();

		static std::vector<std::string> cachedUserModDirs{};
		static Utils::Time::Timer timeSinceCache{ 0 };
		static void CacheUserModDirs() {
			spdlog::warn("Recaching user mod directories");

			if (!cachedUserModDirs.empty())
				cachedUserModDirs.clear();

			cachedUserModDirs.push_back(userModFilesFullPath);
			try {
				const auto rdi = std::filesystem::recursive_directory_iterator(userModFilesFullPath);
				for (auto entry = std::filesystem::begin(rdi); entry != std::filesystem::end(rdi); ++entry) {
					const std::filesystem::path pathToDir = entry->path();
					if (!std::filesystem::is_directory(pathToDir))
						continue;

					cachedUserModDirs.push_back(pathToDir.string());
				}
			} catch (const std::exception& e) {
				spdlog::error("Exception thrown while caching user mod directories: {}", e.what());
			}
		}

		static LPVOID GetFsOpen() {
			return Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?open@fs@@YAPEAUSFsFile@@V?$string_const@D@ttl@@W4TYPE@EFSMode@@W45FFSOpenFlags@@@Z");
		}
		static DWORD64 detourFsOpen(DWORD64 file, DWORD a2, DWORD a3);
		Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, DWORD, DWORD)> FsOpenHook{ "fs::open", &GetFsOpen, &detourFsOpen };

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
					
					finalPath.replace(0, userModFilesFullPath.size() + 1, ""); // replace entire path until mod folder with nothing
					const char* filePath2 = finalPath.c_str();
					spdlog::warn("Loading user mod file \"{}\"", finalPath.c_str());

					const DWORD64 finalAddr = firstByte != 0x0 ? (reinterpret_cast<DWORD64>(filePath2) | (firstByte << 56)) : reinterpret_cast<DWORD64>(filePath2); // restores first byte of addr if first byte was not 0

					const DWORD64 result = FsOpenHook.pOriginal(finalAddr, a2, a3);
					if (!result) {
						spdlog::error("fs::open returned 0! Something went wrong with loading user mod file \"{}\"!\nPlease make sure the path to the file is no longer than 260 characters!", finalPath.c_str());
					}
					return result;
				}
			} catch (const std::exception& e) {
				spdlog::error("Exception thrown while loading user mod file \"{}\": {}", finalPath.c_str(), e.what());
			}
			return FsOpenHook.pOriginal(file, a2, a3);
		}
#pragma endregion

/*#pragma region FsNativeOpen
		static LPVOID GetFsNativeOpen() {
			return Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?open@native@fs@@YAPEAUSFsFile@@PEAVCFsMount@@V?$string_const@D@ttl@@W4TYPE@EFSMode@@W47FFSOpenFlags@@@Z");
		}
		static DWORD64 detourFsNativeOpen(DWORD64 a1, DWORD64 a2, UINT a3, UINT a4);
		static Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, DWORD64, UINT, UINT)> FsNativeOpenHook{ "FsNativeOpen", &GetFsNativeOpen, &detourFsNativeOpen };

		static DWORD64 detourFsNativeOpen(DWORD64 a1, DWORD64 a2, UINT a3, UINT a4) {
			return FsNativeOpenHook.pOriginal(a1, a2, a3, a4);
		}
#pragma endregion

#pragma region LoadDataPaks
		struct CPath {
		public:
			union {
				const char* gamePath;
				buffer<0x8, const char*> pakPath;
				buffer<0x10, const char*> fullPakPath;
			};
		};
		static LPVOID GetLoadDataPaks() {
			return reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(GetModuleHandleA("filesystem_x64_rwdi.dll")) + 0x29580);
		}
		static bool detourLoadDataPaks(DWORD64 a1, DWORD64 a2, DWORD64 a3);
		static Utils::Hook::MHook<LPVOID, bool(*)(DWORD64, DWORD64, DWORD64)> LoadDataPaksHook{ "LoadDataPaks", &GetLoadDataPaks, &detourLoadDataPaks };

		static bool detourLoadDataPaks(DWORD64 a1, DWORD64 a2, DWORD64 a3) {
			CPath* cPath = reinterpret_cast<CPath*>(a2);
			if ((cPath->fullPakPath & 0x1FFFFFFFFFFFFFFF) != 0) {
				const DWORD64 firstByte = (cPath->fullPakPath >> 56) & 0xFF; // get first byte of addr
				std::string gamePath = reinterpret_cast<const char*>(reinterpret_cast<DWORD64>(cPath->gamePath) & 0x1FFFFFFFFFFFFFFF);
				DWORD64 origPakPath = reinterpret_cast<DWORD64>(const_cast<char*>(cPath->pakPath.data));
				std::string pakPath = reinterpret_cast<const char*>(cPath->pakPath & 0x1FFFFFFFFFFFFFFF);
				DWORD64 origFullPakPath = reinterpret_cast<DWORD64>(const_cast<char*>(cPath->fullPakPath.data));
				std::string fullPakPath = reinterpret_cast<const char*>(cPath->fullPakPath & 0x1FFFFFFFFFFFFFFF);

				if (pakPath == "ph/source/data0.pak") {
					pakPath = "ph/source/data/EGameTools/UserModFiles/data8.pak";
					const char* pakPathCStr = pakPath.c_str();
					fullPakPath.replace(gamePath.size() + 1, fullPakPath.size(), "ph/source/data/EGameTools/UserModFiles/data8.pak");
					const char* fullPakPathCStr = fullPakPath.c_str();

					const DWORD64 finalAddrPakPath = firstByte != 0x0 ? (reinterpret_cast<DWORD64>(pakPathCStr) | (firstByte << 56)) : reinterpret_cast<DWORD64>(pakPathCStr);
					const DWORD64 finalAddrFullPakPath= firstByte != 0x0 ? (reinterpret_cast<DWORD64>(fullPakPathCStr) | (firstByte << 56)) : reinterpret_cast<DWORD64>(fullPakPathCStr);

					cPath->pakPath = reinterpret_cast<const char*>(finalAddrPakPath);
					cPath->fullPakPath = reinterpret_cast<const char*>(finalAddrFullPakPath);
					bool ret = LoadDataPaksHook.pOriginal(a1, a2, a3);
					cPath->pakPath = reinterpret_cast<const char*>(origPakPath);
					cPath->fullPakPath = reinterpret_cast<const char*>(origFullPakPath);
				}

				int i = 0;
			}
			
			return LoadDataPaksHook.pOriginal(a1, a2, a3);
		}
#pragma endregion*/

#pragma region MountDataPaks
		static DWORD64 detourMountDataPaks(DWORD64 a1, UINT a2, UINT a3, DWORD64* a4, DWORD64(*a5)(DWORD64, DWORD, DWORD64, char*, int), INT16 a6, DWORD64 a7, UINT a8);
		Utils::Hook::MHook<LPVOID, DWORD64(*)(DWORD64, UINT, UINT, DWORD64*, DWORD64(*)(DWORD64, DWORD, DWORD64, char*, int), INT16, DWORD64, UINT)> MountDataPaksHook{ "MountDataPaks", &Offsets::Get_MountDataPaks, &detourMountDataPaks };

		static DWORD64 detourMountDataPaks(DWORD64 a1, UINT a2, UINT a3, DWORD64* a4, DWORD64(*a5)(DWORD64, DWORD, DWORD64, char*, int), INT16 a6, DWORD64 a7, UINT a8) {
			static int i = 0;
			if (a8 == 8)
				i++;
			else if (i < 3)
				spdlog::error("MountDataPaks hook ran less than 3 times with the data PAKs limit set to 8. This means the increased data PAKs limit might not work correctly! If this error message appears and your data PAKs past \"data7.pak\" have not loaded, please contact author.");

			if (Menu::Misc::increaseDataPAKsLimit.GetValue())
				a8 = 200;
			return MountDataPaksHook.pOriginal(a1, a2, a3, a4, a5, a6, a7, a8);
		}
#pragma endregion

#pragma region FsCheckZipCrc
		static LPVOID GetFsCheckZipCrc() {
			return Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?check_zip_crc@izipped_buffer_file@fs@@QEAA_NXZ");
		}
		static bool detourFsCheckZipCrc(LPVOID instance);
		Utils::Hook::MHook<LPVOID, bool(*)(LPVOID)> FsCheckZipCrcHook{ "FsCheckZipCrc", &GetFsCheckZipCrc, &detourFsCheckZipCrc };

		static bool detourFsCheckZipCrc(LPVOID instance) {
			return Menu::Misc::disableSavegameCRCCheck.GetValue() ? true : FsCheckZipCrcHook.pOriginal(instance);
		}
#pragma endregion

#pragma region AuthenticateDataAddNewFile
		static LPVOID GetAuthenticateDataAddNewFile() {
			return Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?AddNewFile@Results@AuthenticateData@@QEAAAEAVFile@12@XZ");
		}
		static LPVOID detourAuthenticateDataAddNewFile(LPVOID instance);
		Utils::Hook::MHook<LPVOID, LPVOID(*)(LPVOID)> AuthenticateDataAddNewFileHook{ "AuthenticateDataAddNewFile", &GetAuthenticateDataAddNewFile, &detourAuthenticateDataAddNewFile };

		static LPVOID detourAuthenticateDataAddNewFile(LPVOID instance) {
			LPVOID result = AuthenticateDataAddNewFileHook.pOriginal(instance);
			if (Menu::Misc::disableDataPAKsCRCCheck.GetValue())
				AuthenticateDataResultsClear(instance);
			return result;
		}
#pragma endregion
	}
}