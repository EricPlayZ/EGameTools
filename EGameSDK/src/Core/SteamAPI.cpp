#include <EGSDK\Core\SteamAPI.h>
#include <EGSDK\Utils\Memory.h>
#include <EGSDK\Utils\WinMemory.h>

namespace EGSDK::Core {
	ISteamClient* SteamAPI::SteamClient() {
		return Utils::Memory::SafeCallFunction("steam_api64.dll", "SteamClient", nullptr);
	}
	HSteamUser SteamAPI::GetHSteamUser() {
		return Utils::Memory::SafeCallFunction("steam_api64.dll", "SteamAPI_GetHSteamUser", -1);
	}
	HSteamPipe SteamAPI::GetHSteamPipe() {
		return Utils::Memory::SafeCallFunction("steam_api64.dll", "SteamAPI_GetHSteamPipe", -1);
	}

	ISteamUser* SteamAPI::GetISteamUser() {
		auto steamClient = SteamClient();
		if (!steamClient)
			return nullptr;

		auto hSteamUser = SteamAPI::GetHSteamUser();
		auto hSteamPipe = SteamAPI::GetHSteamPipe();
		if (!hSteamUser || !hSteamPipe)
			return {};

		return steamClient->GetISteamUser(hSteamUser, hSteamPipe, STEAMUSER_INTERFACE_VERSION);
	}
	ISteamUtils* SteamAPI::GetISteamUtils() {
		auto steamClient = SteamClient();
		if (!steamClient)
			return nullptr;

		auto hSteamPipe = SteamAPI::GetHSteamPipe();
		if (!hSteamPipe)
			return {};

		return steamClient->GetISteamUtils(hSteamPipe, STEAMUTILS_INTERFACE_VERSION);
	}
}