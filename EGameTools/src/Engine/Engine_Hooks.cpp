#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>
#include <EGSDK\Utils\Time.h>
#include <EGSDK\Utils\WinMemory.h>
#include <EGSDK\GamePH\LevelDI.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\Engine\CBaseCamera.h>
#include <EGSDK\Engine\Engine_Misc.h>
#include <EGSDK\GamePH\FreeCamera.h>
#include <EGSDK\Offsets.h>
#include <EGT\Config\ConfigPaths.h>
#include <EGT\Engine\Engine_Hooks.h>
#include <EGT\GamePH\Camera\CameraDollyRuntime.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Misc.h>

namespace EGT::Engine {
	namespace Hooks {
#pragma region MoveCameraFromForwardUpPos
		static bool switchedFreeCamByGamePause = false;
		static vec3 freeCamPosBeforeGamePause{};
		static vec3 freeCamTargetDirBeforeGamePause{};
		static vec3 freeCamUpDirBeforeGamePause{};

		static EGSDK::Utils::Hook::MHook<void*, void(*)(void*, vec3*, vec3*, vec3*), void*, vec3*, vec3*, vec3*> MoveCameraFromForwardUpPosHook{ "MoveCameraFromForwardUpPos", &EGSDK::OffsetManager::Get_MoveCameraFromForwardUpPos, [](void* pCBaseCamera, vec3* targetDirection, vec3* upDirection, vec3* pos) -> void {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);
			if (iLevel->IsTimerFrozen()) {
				switchedFreeCamByGamePause = Menu::Camera::freeCam.GetValue() && iLevel->IsTimerFrozen();
				return MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);
			}
			if (!targetDirection || !upDirection || !pos)
				return;

			auto viewCam = static_cast<EGSDK::Engine::CBaseCamera*>(iLevel->GetViewCamera());
			if (!viewCam)
				return MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);

			if (Menu::Camera::freeCam.GetValue() && viewCam == EGSDK::GamePH::FreeCamera::Get()) {
				if (switchedFreeCamByGamePause) {
					switchedFreeCamByGamePause = false;
					*pos = freeCamPosBeforeGamePause;
					*targetDirection = freeCamTargetDirBeforeGamePause;
					*upDirection = freeCamUpDirBeforeGamePause;
				} else {
					freeCamPosBeforeGamePause = *pos;
					freeCamTargetDirBeforeGamePause = *targetDirection;
					freeCamUpDirBeforeGamePause = *upDirection;
				}
				return MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);
			}

			if ((!Menu::Camera::thirdPersonCamera.GetValue() && Menu::Camera::cameraOffset.isDefault()) || Menu::Camera::photoMode.GetValue() || Menu::Camera::freeCam.GetValue())
				return MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);

			vec3 forwardVec, upVec, leftVec = {};
			viewCam->GetForwardVector(&forwardVec);
			viewCam->GetUpVector(&upVec);
			viewCam->GetLeftVector(&leftVec);

			const auto normForwardVec = forwardVec.normalize();
			const auto normUpVec = upVec.normalize();
			const auto normLeftVec = leftVec.normalize();

			vec3 newCamPos = *pos;

			if (!Menu::Camera::cameraOffset.isDefault() && !Menu::Camera::thirdPersonCamera.GetValue()) {
				newCamPos -= normLeftVec * Menu::Camera::cameraOffset.X;
				newCamPos.Y += Menu::Camera::cameraOffset.Y;
				newCamPos -= normForwardVec * Menu::Camera::cameraOffset.Z;
			} else if (Menu::Camera::thirdPersonCamera.GetValue()) {
				newCamPos -= normForwardVec * -Menu::Camera::thirdPersonDistanceBehindPlayer;
				newCamPos.Y += Menu::Camera::thirdPersonHeightAbovePlayer - 1.5f;
				newCamPos -= normLeftVec * Menu::Camera::thirdPersonHorizontalDistanceFromPlayer;
			}

			*pos = newCamPos;
			MoveCameraFromForwardUpPosHook.ExecuteCallbacksWithOriginal(pCBaseCamera, targetDirection, upDirection, pos);
		} };
#pragma endregion

#pragma region SetFOV
		static void* GetSetFOV() {
			return EGSDK::Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?SetFOV@IBaseCamera@@QEAAXM@Z");
		}
		static EGSDK::Utils::Hook::MHook<void*, void(*)(void*, float), void*, float> SetFOVHook{ "SetFOV", &GetSetFOV, [](void* pCBaseCamera, float fov) -> void {
			auto iLevel = EGSDK::GamePH::LevelDI::Get();
			if (!iLevel || !iLevel->IsLoaded())
				return SetFOVHook.ExecuteCallbacksWithOriginal(pCBaseCamera, fov);
			const auto* freeCam = EGSDK::GamePH::FreeCamera::Get();
			const bool isFreeCamObject = freeCam && pCBaseCamera == freeCam;
			if (Menu::Camera::dollyCamEnabled.GetValue() && Menu::Camera::freeCam.GetValue() && !Menu::Camera::dollyKeyframes.empty()) {
				const bool shouldForceDollyFov = Menu::Camera::dollyPlaying.GetValue() || (!Menu::Camera::dollyRecording.GetValue() && Menu::menuToggle.GetValue());
				if (shouldForceDollyFov) {
					EGT::Menu::Camera::DollyKeyframe evaluated{};
					if (EGT::GamePH::Camera::EvaluateDollyKeyframeAtTime(Menu::Camera::dollyTimelineTime, evaluated))
						fov = evaluated.fov;
				}
			} else if (Menu::Camera::freeCam.GetValue() && isFreeCamObject)
				fov = Menu::Camera::freeCamFOV;
			if (Menu::Camera::thirdPersonCamera.GetValue())
				fov = static_cast<float>(Menu::Camera::thirdPersonFOV);
			else if (!Menu::Camera::firstPersonZoomIn.IsKeyDown() && !EGSDK::Engine::IBaseCamera::isSetFOVCalledByEGSDK)
				Menu::Camera::originalFirstPersonFOVBeforeZoomIn = std::roundf(fov);

			return SetFOVHook.ExecuteCallbacksWithOriginal(pCBaseCamera, fov);
		} };
#pragma endregion

#pragma region fs::open
		static const std::string userModFilesFullPath = "\\\\?\\" + Config::Paths::GetUserModFilesDir().string();
		static std::vector<std::string> cachedUserModDirs{};
		static EGSDK::Utils::Time::Timer cacheUpdateTimer{ 0 };

		static void RecacheUserModDirectories() {
			SPDLOG_INFO("Recaching user mod directories");
			
			try {
				cachedUserModDirs.clear();
				cachedUserModDirs.push_back(userModFilesFullPath);

				for (const auto& entry : std::filesystem::recursive_directory_iterator(userModFilesFullPath)) {
					if (entry.is_directory())
						cachedUserModDirs.push_back(entry.path().string());
				}
			} catch (const std::exception& e) {
				SPDLOG_ERROR("Exception thrown while caching user mod directories: {}", e.what());
			}
		}

		static std::optional<std::string> FindValidModFilePath(const std::string& fileName) {
			for (const auto& dir : cachedUserModDirs) {
				std::string potentialPath = dir + "\\" + fileName;
				if (std::filesystem::exists(potentialPath)) {
					SPDLOG_INFO("Found user mod file: {}", potentialPath);
					return potentialPath.substr(userModFilesFullPath.size() + 1); // Strip base path
				}
			}
			return std::nullopt;
		}

		static void* GetFsOpen() {
			return EGSDK::Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?open@fs@@YAPEAUSFsFile@@V?$string_const@D@ttl@@W4TYPE@EFSMode@@W45FFSOpenFlags@@@Z");
		}
		EGSDK::Utils::Hook::MHook<void*, uint64_t(*)(uint64_t, DWORD, DWORD), uint64_t, DWORD, DWORD> FsOpenHook{ "fs::open", &GetFsOpen, [](uint64_t file, DWORD a2, DWORD a3) -> uint64_t {
			const uint64_t firstByte = (file >> 56) & 0xFF; // get first byte of addr
			const char* filePath = reinterpret_cast<const char*>(file & 0x1FFFFFFFFFFFFFFF); // remove first byte of addr in case it exists
			const std::string fileName = std::filesystem::path(filePath).filename().string();
			if (fileName.empty() || fileName.contains("EGameTools") || fileName.ends_with(".model"))
				return FsOpenHook.ExecuteCallbacksWithOriginal(file, a2, a3);

			if (cacheUpdateTimer.DidTimePass()) {
				RecacheUserModDirectories();
				cacheUpdateTimer = EGSDK::Utils::Time::Timer(5000);
			}

			std::string finalPath{};
			try {
				if (auto modFilePath = FindValidModFilePath(fileName)) {
					const char* modFileCStr = modFilePath->c_str();
					SPDLOG_INFO("Loading user mod file: {}", modFilePath->c_str());

					uint64_t modFileAddr = reinterpret_cast<uint64_t>(modFileCStr);
					if (firstByte != 0x0)
						modFileAddr |= (firstByte << 56);

					uint64_t result = FsOpenHook.ExecuteCallbacksWithOriginal(modFileAddr, a2, a3);
					if (!result) {
						SPDLOG_ERROR("fs::open returned 0! Something went wrong with loading user mod file \"{}\"!\nPlease make sure the path to the file is no longer than 260 characters!", finalPath.c_str());
						return FsOpenHook.ExecuteCallbacksWithOriginal(file, a2, a3);
					}
				}
			} catch (const std::exception& e) {
				SPDLOG_ERROR("Exception thrown while loading user mod file \"{}\": {}", finalPath.c_str(), e.what());
			}
			return FsOpenHook.ExecuteCallbacksWithOriginal(file, a2, a3);
		} };
#pragma endregion

		// Thank you @12brendon34 on Discord for help with finding the function responsible for .PAK loading!
#pragma region CResourceLoadingRuntimeCreate
		namespace fs {
			struct mount_path {
				union {
					const char* gamePath;
					EGSDK::ClassHelpers::StaticBuffer<0x8, const char*> pakPath;
					EGSDK::ClassHelpers::StaticBuffer<0x10, const char*> fullPakPath;
				};
			};

			static uint64_t mount(mount_path* path, USHORT flags, void** a3) {
				return EGSDK::Utils::Memory::SafeCallFunction<uint64_t>("filesystem_x64_rwdi.dll", "?mount@fs@@YA_NAEBUmount_path@1@GPEAPEAVCFsMount@@@Z", 0, path, flags, a3);
			}
		}

		static bool MountUserPakFile(const std::string& fullPakPath, const std::string& gamePath) {
			try {
				std::string pakPath = fullPakPath;
				pakPath.erase(0, gamePath.size() + 1);

				fs::mount_path path{};
				path.gamePath = gamePath.c_str();
				path.pakPath = pakPath.c_str();
				path.fullPakPath = fullPakPath.c_str();

				SPDLOG_INFO("Attempting to load user PAK mod file: \"{}\"", pakPath);

				// Attempt to mount the PAK file
				if (!fs::mount(&path, 1, nullptr)) {
					SPDLOG_ERROR("Failed to mount user PAK mod file \"{}\". Ensure the file is valid and the path is no longer than 260 characters.", pakPath);
					return false;
				}

				SPDLOG_INFO("Successfully loaded user PAK mod file: \"{}\"", pakPath);
				return true;
			} catch (const std::exception& e) {
				SPDLOG_ERROR("Error mounting user PAK mod file \"{}\": {}", fullPakPath, e.what());
				return false;
			}
		}
		static void IterateAndMountUserPakFiles(const std::string& userModFilesPath, const std::string& gamePath) {
			try {
				for (const auto& entry : std::filesystem::recursive_directory_iterator(userModFilesPath)) {
					if (entry.is_directory())
						continue;

					const std::string fullPakPath = entry.path().string();
					if (EGSDK::Utils::Values::str_ends_with_ci(fullPakPath, ".pak"))
						MountUserPakFile(fullPakPath, gamePath);
				}
			} catch (const std::exception& e) {
				SPDLOG_ERROR("Exception during PAK file iteration: {}", e.what());
			}
		}

		static void* GetCResourceLoadingRuntimeCreate() {
			return EGSDK::Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?Create@CResourceLoadingRuntime@@SAPEAV1@_N@Z");
		}
		static EGSDK::Utils::Hook::MHook<void*, void* (*)(bool), bool> CResourceLoadingRuntimeCreateHook{ "CResourceLoadingRuntimeCreate", &GetCResourceLoadingRuntimeCreate, [](bool noTexStreaming) -> void* {
			std::string gamePath = "\\\\?\\" + Config::Paths::GetGameRootDir().string();

			IterateAndMountUserPakFiles(userModFilesFullPath, gamePath);

			return CResourceLoadingRuntimeCreateHook.ExecuteCallbacksWithOriginal(noTexStreaming);
		} };
#pragma endregion

#pragma region MountDataPaks
		int mountDataPaksRanWith8Count = 0;

		EGSDK::Utils::Hook::MHook<void*, uint64_t(*)(uint64_t, UINT, UINT, uint64_t*, uint64_t(*)(uint64_t, DWORD, uint64_t, char*, int), INT16, uint64_t, UINT), uint64_t, UINT, UINT, uint64_t*, uint64_t(*)(uint64_t, DWORD, uint64_t, char*, int), INT16, uint64_t, UINT> MountDataPaksHook{ "MountDataPaks", &EGSDK::OffsetManager::Get_MountDataPaks, [](uint64_t a1, UINT a2, UINT a3, uint64_t* a4, uint64_t(*a5)(uint64_t, DWORD, uint64_t, char*, int), INT16 a6, uint64_t a7, UINT a8) -> uint64_t {
			if (Menu::Misc::increaseDataPAKsLimit.GetValue()) {
				if (a8 == 8)
					mountDataPaksRanWith8Count++;
				a8 = 200;
			}
			return MountDataPaksHook.ExecuteOriginal(a1, a2, a3, a4, a5, a6, a7, a8);
		} };
#pragma endregion

#pragma region FsCheckZipCrc
		static void* GetFsCheckZipCrc() {
			return EGSDK::Utils::Memory::GetProcAddr("filesystem_x64_rwdi.dll", "?check_zip_crc@izipped_buffer_file@fs@@QEAA_NXZ");
		}
		EGSDK::Utils::Hook::MHook<void*, bool(*)(void*), void*> FsCheckZipCrcHook{ "FsCheckZipCrc", &GetFsCheckZipCrc, [](void* instance) -> bool {
			return Menu::Misc::disableSavegameCRCCheck.GetValue() ? true : FsCheckZipCrcHook.ExecuteCallbacksWithOriginal(instance);
		} };
#pragma endregion

#pragma region AuthenticateDataAddNewFile
		static void* GetAuthenticateDataAddNewFile() {
			return EGSDK::Utils::Memory::GetProcAddr("engine_x64_rwdi.dll", "?AddNewFile@Results@AuthenticateData@@QEAAAEAVFile@12@XZ");
		}
		EGSDK::Utils::Hook::MHook<void*, void* (*)(void*), void*> AuthenticateDataAddNewFileHook{ "AuthenticateDataAddNewFile", &GetAuthenticateDataAddNewFile, [](void* instance) -> void* {
			void* result = AuthenticateDataAddNewFileHook.ExecuteCallbacksWithOriginal(instance);
			if (Menu::Misc::disableDataPAKsCRCCheck.GetValue())
				EGSDK::Engine::AuthenticateDataResultsClear(instance);
			return result;
		} };
#pragma endregion

#pragma region GetCVarT
		EGSDK::Utils::Hook::MHook<void*, EGSDK::Engine::CVar*(*)(uint64_t, const char*, uint64_t, int), uint64_t, const char*, uint64_t, int> GetCVarTHook{ "GetCVarT", &EGSDK::OffsetManager::Get_GetCVarT, [](uint64_t pEvent, const char* name, uint64_t eventType, int someFlag) -> EGSDK::Engine::CVar* {
			EGSDK::Engine::CVar* cVarPtr = GetCVarTHook.ExecuteOriginal(pEvent, name, eventType, someFlag);
			if (!cVarPtr)
				return cVarPtr;
			if (EGSDK::Engine::CVars::vars.Find(name))
				return cVarPtr;

			switch (eventType - pEvent) {
				case 0x1A8:
				{
					cVarPtr->SetName(name);
					cVarPtr->SetType(EGSDK::Engine::VarType::Float);
					EGSDK::Engine::CVars::vars.AddVar(std::unique_ptr<EGSDK::Engine::CVar>(cVarPtr));
					break;
				}
				case 0x198:
				{
					cVarPtr->SetName(name);
					cVarPtr->SetType(EGSDK::Engine::VarType::Int);
					EGSDK::Engine::CVars::vars.AddVar(std::unique_ptr<EGSDK::Engine::CVar>(cVarPtr));
					break;
				}
				case 0x1C8:
				{
					cVarPtr->SetName(name);
					cVarPtr->SetType(EGSDK::Engine::VarType::Vec3);
					EGSDK::Engine::CVars::vars.AddVar(std::unique_ptr<EGSDK::Engine::CVar>(cVarPtr));
					break;
				}
				case 0x1D8:
				{
					cVarPtr->SetName(name);
					cVarPtr->SetType(EGSDK::Engine::VarType::Vec4);
					EGSDK::Engine::CVars::vars.AddVar(std::unique_ptr<EGSDK::Engine::CVar>(cVarPtr));
					break;
				}
				default:
					break;
			}

			return cVarPtr;
		} };
#pragma endregion

#pragma region GetCVarValue
		static std::unordered_map<uint32_t, uint64_t*> cVarValuePtrs;
		static EGSDK::Utils::Hook::MHook<void*, uint64_t*(*)(uint64_t, uint32_t, int), uint64_t, uint32_t, int> GetCVarValueHook{ "GetCVarValue", &EGSDK::OffsetManager::Get_GetCVarValue, [](uint64_t a1, uint32_t valueOffset, int a3) -> uint64_t* {
			uint64_t* varValuePtr = GetCVarValueHook.ExecuteOriginal(a1, valueOffset, a3);
			if (!varValuePtr)
				return varValuePtr;

			auto cVar = EGSDK::Engine::CVars::GetVarRef(valueOffset);
			if (!cVar)
				return varValuePtr;
			cVar->AddValuePtr(varValuePtr);

			//EGSDK::Engine::CVar* customCVar = EGSDK::Engine::CVars::customVars.Find(cVar->GetName());
			auto customCVar = EGSDK::Engine::CVars::GetCustomVarRef(cVar->GetName());
			if (!customCVar)
				return varValuePtr;

			switch (cVar->GetType()) {
				case EGSDK::Engine::VarType::Float:
				{
					auto value = customCVar->GetValue<float>();
					if (value)
						cVar->SetValue(*value);
					break;
				}
				case EGSDK::Engine::VarType::Int:
				{
					auto value = customCVar->GetValue<int>();
					if (value)
						cVar->SetValue(*value);
					break;
				}
				case EGSDK::Engine::VarType::Vec3:
				{
					auto value = customCVar->GetValue<vec3>();
					if (value)
						cVar->SetValue(*value);
					break;
				}
				case EGSDK::Engine::VarType::Vec4:
				{
					auto value = customCVar->GetValue<vec4>();
					if (value)
						cVar->SetValue(*value);
					break;
				}
			}

			return varValuePtr;
		} };
#pragma endregion

#pragma region HandleTimeWeatherInterpolationOnDemandTextureIsLoaded
		static unsigned char HandleTimeWeatherInterpolationOnDemandTextureIsLoadedBytes[9] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }; // nop
		EGSDK::Utils::Hook::ByteHook<void*> HandleTimeWeatherInterpolationOnDemandTextureIsLoadedHook{ "HandleTimeWeatherInterpolationOnDemandTextureIsLoaded", &EGSDK::OffsetManager::Get_HandleTimeWeatherInterpolationOnDemandTextureIsLoaded, HandleTimeWeatherInterpolationOnDemandTextureIsLoadedBytes, sizeof(HandleTimeWeatherInterpolationOnDemandTextureIsLoadedBytes), false }; // test bpl, bpl; jz loc_CFA242
#pragma endregion
	}
}