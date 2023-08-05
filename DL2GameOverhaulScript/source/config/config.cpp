#include <filesystem>
#include "..\menu\player.h"
#include "..\menu\camera.h"
#include "..\utils.h"
#include "..\print.h"
#include "ini.h"

namespace Config {
	static const std::unordered_map<std::string_view, int> virtualKeyCodes = {
		// Function keys
		{ "VK_F1", VK_F1 },
		{ "VK_F2", VK_F2 },
		{ "VK_F3", VK_F3 },
		{ "VK_F4", VK_F4 },
		{ "VK_F5", VK_F5 },
		{ "VK_F6", VK_F6 },
		{ "VK_F7", VK_F7 },
		{ "VK_F8", VK_F8 },
		{ "VK_F9", VK_F9 },
		{ "VK_F10", VK_F10 },
		{ "VK_F11", VK_F11 },
		{ "VK_F12", VK_F12 },
		{ "VK_F13", VK_F13 },
		{ "VK_F14", VK_F14 },
		{ "VK_F15", VK_F15 },
		{ "VK_F16", VK_F16 },
		{ "VK_F17", VK_F17 },
		{ "VK_F18", VK_F18 },
		{ "VK_F19", VK_F19 },
		{ "VK_F20", VK_F20 },
		{ "VK_F21", VK_F21 },
		{ "VK_F22", VK_F22 },
		{ "VK_F23", VK_F23 },
		{ "VK_F24", VK_F24 },

		// Number keys
		{ "VK_0", 0x30 },
		{ "VK_1", 0x31 },
		{ "VK_2", 0x32 },
		{ "VK_3", 0x33 },
		{ "VK_4", 0x34 },
		{ "VK_5", 0x35 },
		{ "VK_6", 0x36 },
		{ "VK_7", 0x37 },
		{ "VK_8", 0x38 },
		{ "VK_9", 0x39 },
		{ "0", 0x30 },
		{ "1", 0x31 },
		{ "2", 0x32 },
		{ "3", 0x33 },
		{ "4", 0x34 },
		{ "5", 0x35 },
		{ "6", 0x36 },
		{ "7", 0x37 },
		{ "8", 0x38 },
		{ "9", 0x39 },

		// Alphabetic keys
		{ "VK_A", 0x41 },
		{ "VK_B", 0x42 },
		{ "VK_C", 0x43 },
		{ "VK_D", 0x44 },
		{ "VK_E", 0x45 },
		{ "VK_F", 0x46 },
		{ "VK_G", 0x47 },
		{ "VK_H", 0x48 },
		{ "VK_I", 0x49 },
		{ "VK_J", 0x4A },
		{ "VK_K", 0x4B },
		{ "VK_L", 0x4C },
		{ "VK_M", 0x4D },
		{ "VK_N", 0x4E },
		{ "VK_O", 0x4F },
		{ "VK_P", 0x50 },
		{ "VK_Q", 0x51 },
		{ "VK_R", 0x52 },
		{ "VK_S", 0x53 },
		{ "VK_T", 0x54 },
		{ "VK_U", 0x55 },
		{ "VK_V", 0x56 },
		{ "VK_W", 0x57 },
		{ "VK_X", 0x58 },
		{ "VK_Y", 0x59 },
		{ "VK_Z", 0x5A },
		{ "A", 0x41 },
		{ "B", 0x42 },
		{ "C", 0x43 },
		{ "D", 0x44 },
		{ "E", 0x45 },
		{ "F", 0x46 },
		{ "G", 0x47 },
		{ "H", 0x48 },
		{ "I", 0x49 },
		{ "J", 0x4A },
		{ "K", 0x4B },
		{ "L", 0x4C },
		{ "M", 0x4D },
		{ "N", 0x4E },
		{ "O", 0x4F },
		{ "P", 0x50 },
		{ "Q", 0x51 },
		{ "R", 0x52 },
		{ "S", 0x53 },
		{ "T", 0x54 },
		{ "U", 0x55 },
		{ "V", 0x56 },
		{ "W", 0x57 },
		{ "X", 0x58 },
		{ "Y", 0x59 },
		{ "Z", 0x5A },

		// Special keys
		{"VK_BACK", VK_BACK },
		{"VK_TAB", VK_TAB },
		{"VK_CLEAR", VK_CLEAR },
		{"VK_RETURN", VK_RETURN },
		{"VK_SHIFT", VK_SHIFT },
		{"VK_CONTROL", VK_CONTROL },
		{"VK_MENU", VK_MENU },
		{"VK_PAUSE", VK_PAUSE },
		{"VK_CAPITAL", VK_CAPITAL },
		{"VK_ESCAPE", VK_ESCAPE },
		{"VK_SPACE", VK_SPACE },
		{"VK_PRIOR", VK_PRIOR },
		{"VK_NEXT", VK_NEXT },
		{"VK_END", VK_END },
		{"VK_HOME", VK_HOME },
		{"VK_LEFT", VK_LEFT },
		{"VK_UP", VK_UP },
		{"VK_RIGHT", VK_RIGHT },
		{"VK_DOWN", VK_DOWN },
		{"VK_SELECT", VK_SELECT },
		{"VK_PRINT", VK_PRINT },
		{"VK_EXECUTE", VK_EXECUTE },
		{"VK_SNAPSHOT", VK_SNAPSHOT },
		{"VK_INSERT", VK_INSERT },
		{"VK_DELETE", VK_DELETE },
		{"VK_HELP", VK_HELP },

		// Numpad keys
		{ "VK_NUMPAD0", VK_NUMPAD0 },
		{ "VK_NUMPAD1", VK_NUMPAD1 },
		{ "VK_NUMPAD2", VK_NUMPAD2 },
		{ "VK_NUMPAD3", VK_NUMPAD3 },
		{ "VK_NUMPAD4", VK_NUMPAD4 },
		{ "VK_NUMPAD5", VK_NUMPAD5 },
		{ "VK_NUMPAD6", VK_NUMPAD6 },
		{ "VK_NUMPAD7", VK_NUMPAD7 },
		{ "VK_NUMPAD8", VK_NUMPAD8 },
		{ "VK_NUMPAD9", VK_NUMPAD9 },
		{ "VK_MULTIPLY", VK_MULTIPLY },
		{ "VK_ADD", VK_ADD },
		{ "VK_SEPARATOR", VK_SEPARATOR },
		{ "VK_SUBTRACT", VK_SUBTRACT },
		{ "VK_DECIMAL", VK_DECIMAL },
		{ "VK_DIVIDE", VK_DIVIDE },

		// Modifier keys
		{ "VK_SHIFT", VK_SHIFT },
		{ "VK_LSHIFT", VK_LSHIFT },
		{ "VK_RSHIFT", VK_RSHIFT },
		{ "VK_CONTROL", VK_CONTROL },
		{ "VK_LCONTROL", VK_LCONTROL },
		{ "VK_RCONTROL", VK_RCONTROL },
		{ "VK_MENU", VK_MENU },
		{ "VK_LMENU", VK_LMENU },
		{ "VK_RMENU", VK_RMENU },

		// Browser keys
		{ "VK_BROWSER_BACK", VK_BROWSER_BACK },
		{ "VK_BROWSER_FORWARD", VK_BROWSER_FORWARD },
		{ "VK_BROWSER_REFRESH", VK_BROWSER_REFRESH },
		{ "VK_BROWSER_STOP", VK_BROWSER_STOP },
		{ "VK_BROWSER_SEARCH", VK_BROWSER_SEARCH },
		{ "VK_BROWSER_FAVORITES", VK_BROWSER_FAVORITES },
		{ "VK_BROWSER_HOME", VK_BROWSER_HOME },

		// Volume keys
		{ "VK_VOLUME_MUTE", VK_VOLUME_MUTE },
		{ "VK_VOLUME_DOWN", VK_VOLUME_DOWN },
		{ "VK_VOLUME_UP", VK_VOLUME_UP },

		// Media keys
		{ "VK_MEDIA_NEXT_TRACK", VK_MEDIA_NEXT_TRACK },
		{ "VK_MEDIA_PREV_TRACK", VK_MEDIA_PREV_TRACK },
		{ "VK_MEDIA_STOP", VK_MEDIA_STOP },
		{ "VK_MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE },

		// Application keys
		{ "VK_LAUNCH_MAIL", VK_LAUNCH_MAIL },
		{ "VK_LAUNCH_MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT },
		{ "VK_LAUNCH_APP1", VK_LAUNCH_APP1 },
		{ "VK_LAUNCH_APP2", VK_LAUNCH_APP2 },

		// Other keys
		{ "VK_OEM_1", VK_OEM_1 },
		{ "VK_OEM_PLUS", VK_OEM_PLUS },
		{ "VK_OEM_COMMA", VK_OEM_COMMA },
		{ "VK_OEM_MINUS", VK_OEM_MINUS },
		{ "VK_OEM_PERIOD", VK_OEM_PERIOD },
		{ "VK_OEM_2", VK_OEM_2 },
		{ "VK_OEM_3", VK_OEM_3 },
		{ "VK_OEM_4", VK_OEM_4 },
		{ "VK_OEM_5", VK_OEM_5 },
		{ "VK_OEM_6", VK_OEM_6 },
		{ "VK_OEM_7", VK_OEM_7 },
		{ "VK_OEM_8", VK_OEM_8 },
		{ "VK_OEM_102", VK_OEM_102 },
		{ "VK_PROCESSKEY", VK_PROCESSKEY },
		{ "VK_PACKET", VK_PACKET },
		{ "VK_ATTN", VK_ATTN },
		{ "VK_CRSEL", VK_CRSEL },
		{ "VK_EXSEL", VK_EXSEL },
		{ "VK_EREOF", VK_EREOF },
		{ "VK_PLAY", VK_PLAY },
		{ "VK_ZOOM", VK_ZOOM },
		{ "VK_NONAME", VK_NONAME },
		{ "VK_PA1", VK_PA1 },
		{ "VK_OEM_CLEAR", VK_OEM_CLEAR }
	};

	static const char* configFileName = "EGameTools.ini";
	static std::filesystem::file_time_type configPreviousWriteTime{};
	static std::filesystem::file_time_type configLastWriteTime{};

	std::string configStatus{};
	std::string configError{};
	static bool savedConfig = false;

	static inih::INIReader reader{};
	int menuToggleKey = VK_F5;

	static void LoadDefaultConfig() {
		reader = inih::INIReader();

		reader.InsertEntry("Menu:Keybinds", "ToggleKey", "VK_F5");
		menuToggleKey = VK_F5;

		const std::filesystem::path desktopPath = Utils::GetDesktopDir();
		std::string desktopPathStr = desktopPath.string();
		if (!desktopPath.empty() && !(std::filesystem::is_directory(desktopPath.parent_path()) && std::filesystem::is_directory(desktopPath)))
			desktopPathStr = {};

		reader.InsertEntry("Player:PlayerVariables", "LastSaveSCRPath", desktopPathStr);
		Menu::Player::saveSCRPath = desktopPathStr;

		const std::string loadSCRFilePath = desktopPathStr.empty() ? "" : desktopPathStr + "\\player_variables.scr";

		reader.InsertEntry("Player:PlayerVariables", "LastLoadSCRFilePath", loadSCRFilePath);
		Menu::Player::loadSCRFilePath = loadSCRFilePath;

		reader.InsertEntry("Camera", "DisablePhotoModeLimits", true);
		reader.InsertEntry("Camera", "DisableSafezoneFOVReduction", true);
	}
	static void LoadAndWriteDefaultConfig() {
		LoadDefaultConfig();
		try {
			inih::INIWriter writer{};
			writer.write(configFileName, reader);
		} catch (const std::runtime_error& e) {
			configError = PrintError("Error writing file %s: %s", configFileName, e.what());
		}
	}
	static bool ConfigExists() {
		return std::filesystem::exists(configFileName);
	}
	static void CreateConfig() {
		configStatus = PrintWarning("%s does not exist (will create now); using default config values", configFileName);

		LoadAndWriteDefaultConfig();
	}
	static void ReadConfig(const bool configUpdate = false) {
		try {
			reader = inih::INIReader(configFileName);
			const std::string toggleKey = reader.Get<std::string>("Menu:Keybinds", "ToggleKey");
			auto it = virtualKeyCodes.find(toggleKey);
			if (it != virtualKeyCodes.end())
				menuToggleKey = it->second;
			else
				menuToggleKey = VK_F5;

			const std::filesystem::path desktopPath = Utils::GetDesktopDir();
			std::string desktopPathStr = desktopPath.string();
			if (!desktopPath.empty() && !(std::filesystem::is_directory(desktopPath.parent_path()) && std::filesystem::is_directory(desktopPath)))
				desktopPathStr = {};

			Menu::Player::saveSCRPath = reader.Get<std::string>("Player:PlayerVariables", "LastSaveSCRPath");
			if (Menu::Player::saveSCRPath.empty())
				Menu::Player::saveSCRPath = desktopPathStr;

			const std::filesystem::path saveSCRPath = Menu::Player::saveSCRPath;
			if (!saveSCRPath.empty() && !(std::filesystem::is_directory(saveSCRPath.parent_path()) && std::filesystem::is_directory(saveSCRPath)))
				Menu::Player::saveSCRPath = {};

			const std::string loadSCRFilePathStr = desktopPathStr.empty() ? "" : desktopPathStr + "\\player_variables.scr";

			Menu::Player::loadSCRFilePath = reader.Get<std::string>("Player:PlayerVariables", "LastLoadSCRFilePath");
			if (Menu::Player::loadSCRFilePath.empty())
				Menu::Player::loadSCRFilePath = loadSCRFilePathStr;
			const std::filesystem::path loadSCRFilePath = Menu::Player::loadSCRFilePath;

			if (!loadSCRFilePath.empty() && !std::filesystem::is_directory(loadSCRFilePath.parent_path()))
				Menu::Player::loadSCRFilePath = {};

			Menu::Camera::disablePhotoModeLimitsEnabled.value = reader.Get<bool>("Camera", "DisablePhotoModeLimits", true);
			Menu::Camera::disableSafezoneFOVReductionEnabled.value = reader.Get<bool>("Camera", "DisableSafezoneFOVReduction", true);

			configStatus = PrintSuccess(configUpdate ? "Successfully read updated config!" : "Successfully read config!");
		} catch (const std::runtime_error& e) {
			configError = PrintSuccess("Error writing file %s; using default config values: %s", configFileName, e.what());

			LoadDefaultConfig();
		}
	}

	void SaveConfig() {
		reader.UpdateEntry("Player:PlayerVariables", "LastSaveSCRPath", Menu::Player::saveSCRPath);
		reader.UpdateEntry("Player:PlayerVariables", "LastLoadSCRFilePath", Menu::Player::loadSCRFilePath);

		reader.UpdateEntry("Camera", "DisablePhotoModeLimits", Menu::Camera::disablePhotoModeLimitsEnabled.value);
		reader.UpdateEntry("Camera", "DisableSafezoneFOVReduction", Menu::Camera::disableSafezoneFOVReductionEnabled.value);

		try {
			inih::INIWriter writer{};
			writer.write(configFileName, reader);

			savedConfig = true;
		} catch (const std::runtime_error& e) {
			configError = PrintError("Error saving to file %s: %s", configFileName, e.what());
		}
	}
	void InitConfig() {
		ConfigExists() ? ReadConfig() : CreateConfig();

		configPreviousWriteTime = std::filesystem::last_write_time(configFileName);
		configLastWriteTime = configPreviousWriteTime;
	}
	void ConfigLoop() {
		while (true) {
			if (Core::exiting)
				return;

			Sleep(200);

			if (!ConfigExists()) {
				CreateConfig();
				Sleep(200);
				configPreviousWriteTime = std::filesystem::last_write_time(configFileName);
			}

			// Check for config changes
			configLastWriteTime = std::filesystem::last_write_time(configFileName);
			if (configLastWriteTime != configPreviousWriteTime && !savedConfig) {
				configPreviousWriteTime = configLastWriteTime;

				Sleep(200);
				ReadConfig(true);
			} else if (configLastWriteTime != configPreviousWriteTime && savedConfig) {
				configPreviousWriteTime = configLastWriteTime;
				savedConfig = false;
			}
		}
	}
	void ConfigSaveLoop() {
		while (true) {
			if (Core::exiting)
				return;

			Sleep(10000);

			if (!ConfigExists()) {
				CreateConfig();
				Sleep(200);
				configPreviousWriteTime = std::filesystem::last_write_time(configFileName);
			}

			SaveConfig();
		}
	}
}