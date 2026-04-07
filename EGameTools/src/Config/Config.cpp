#include <Windows.h>
#include <any>
#include <filesystem>
#include <spdlog\spdlog.h>
#include <ini\ini.h>
#include <EGSDK\Utils\Files.h>
#include <EGT\Menu\Camera.h>
#include <EGT\Menu\Menu.h>
#include <EGT\Menu\Misc.h>
#include <EGT\Menu\Player.h>
#include <EGT\Menu\Teleport.h>
#include <EGT\Menu\Weapon.h>
#include <EGT\Menu\World.h>
#include <EGT\Menu\Debug.h>
#include <EGT\Config\Config.h>

namespace EGT::Config {
	enum ValueType {
		OPTION,
		Int,
		Float,
		String
	};
	
	struct ConfigEntry {
		std::string_view section;
		std::string_view key;
		std::any value;
		void* optionPtr;
		ValueType type;
	};
	static std::vector<ConfigEntry> configVariablesDefault{};
	static std::vector<ConfigEntry> configVariables{};
	static constexpr const char* configFileName = "EGameTools.ini";
	static std::filesystem::file_time_type configPreviousWriteTime{};
	static std::filesystem::file_time_type configLastWriteTime{};

	static bool savedConfig = false;

	static inih::INIReader reader{};

	static void InitializeConfigVariables() {
		configVariablesDefault = std::vector<ConfigEntry>({
			{ "Menu", "Opacity", Menu::opacity, &Menu::opacity, Float },
			{ "Menu", "ChildPanelAlpha", Menu::childPanelAlpha, &Menu::childPanelAlpha, Float },
			{ "Menu", "FrameAlpha", Menu::frameAlpha, &Menu::frameAlpha, Float },
			{ "Menu", "PopupAlpha", Menu::popupAlpha, &Menu::popupAlpha, Float },
			{ "Menu", "MicaWashStrength", Menu::micaWashStrength, &Menu::micaWashStrength, Float },
			{ "Menu", "MicaBlurStrength", Menu::micaBlurStrength, &Menu::micaBlurStrength, Float },
			{ "Menu", "Scale", Menu::scale, &Menu::scale, Float },
			{ "Menu", "FirstTimeRunning", Menu::firstTimeRunning.GetValue(), &Menu::firstTimeRunning, OPTION },
			{ "Menu", "HasSeenChangelog", Menu::hasSeenChangelog.GetValue(), &Menu::hasSeenChangelog, OPTION },
			{ "Menu:Keybinds", "MenuToggleKey", Menu::menuToggle.ToStringVKeyMap(), &Menu::menuToggle, String },
			{ "Menu:Keybinds", "GodModeToggleKey", Menu::Player::godMode.ToStringVKeyMap(), &Menu::Player::godMode, String },
			{ "Menu:Keybinds", "FreezePlayerToggleKey", Menu::Player::freezePlayer.ToStringVKeyMap(), &Menu::Player::freezePlayer, String },
			{ "Menu:Keybinds", "UnlimitedImmunityToggleKey", Menu::Player::unlimitedImmunity.ToStringVKeyMap(), &Menu::Player::unlimitedImmunity, String },
			{ "Menu:Keybinds", "UnlimitedStaminaToggleKey", Menu::Player::unlimitedStamina.ToStringVKeyMap(), &Menu::Player::unlimitedStamina, String },
			{ "Menu:Keybinds", "UnlimitedItemsToggleKey", Menu::Player::unlimitedItems.ToStringVKeyMap(), &Menu::Player::unlimitedItems, String },
			{ "Menu:Keybinds", "OneHitKillToggleKey", Menu::Player::oneHitKill.ToStringVKeyMap(), &Menu::Player::oneHitKill, String },
			{ "Menu:Keybinds", "DisableOutOfBoundsTimerToggleKey", Menu::Player::disableOutOfBoundsTimer.ToStringVKeyMap(), &Menu::Player::disableOutOfBoundsTimer, String },
			{ "Menu:Keybinds", "NightrunnerModeToggleKey", Menu::Player::nightrunnerMode.ToStringVKeyMap(), &Menu::Player::nightrunnerMode, String },
			{ "Menu:Keybinds", "OneHandedModeToggleKey", Menu::Player::oneHandedMode.ToStringVKeyMap(), &Menu::Player::oneHandedMode, String },
			{ "Menu:Keybinds", "DisableSafezoneRestrictionsToggleKey", Menu::Player::disableSafezoneRestrictions.ToStringVKeyMap(), &Menu::Player::disableSafezoneRestrictions, String },
			{ "Menu:Keybinds", "DisableAirControlToggleKey", Menu::Player::disableAirControl.ToStringVKeyMap(), &Menu::Player::disableAirControl, String },
			{ "Menu:Keybinds", "UnlimitedDurabilityToggleKey", Menu::Weapon::unlimitedDurability.ToStringVKeyMap(), &Menu::Weapon::unlimitedDurability, String },
			{ "Menu:Keybinds", "UnlimitedAmmoToggleKey", Menu::Weapon::unlimitedAmmo.ToStringVKeyMap(), &Menu::Weapon::unlimitedAmmo, String },
			{ "Menu:Keybinds", "NoSpreadToggleKey", Menu::Weapon::noSpread.ToStringVKeyMap(), &Menu::Weapon::noSpread, String },
			{ "Menu:Keybinds", "NoRecoilToggleKey", Menu::Weapon::noRecoil.ToStringVKeyMap(), &Menu::Weapon::noRecoil, String },
			{ "Menu:Keybinds", "InstantReloadToggleKey", Menu::Weapon::instantReload.ToStringVKeyMap(), &Menu::Weapon::instantReload, String },
			{ "Menu:Keybinds", "FirstPersonZoomInHoldingKey", Menu::Camera::firstPersonZoomIn.ToStringVKeyMap(), &Menu::Camera::firstPersonZoomIn, String },
			{ "Menu:Keybinds", "ThirdPersonToggleKey", Menu::Camera::thirdPersonCamera.ToStringVKeyMap(), &Menu::Camera::thirdPersonCamera, String },
			{ "Menu:Keybinds", "UseTPPModelToggleKey", Menu::Camera::tpUseTPPModel.ToStringVKeyMap(), &Menu::Camera::tpUseTPPModel, String },
			{ "Menu:Keybinds", "FreeCamToggleKey", Menu::Camera::freeCam.ToStringVKeyMap(), &Menu::Camera::freeCam, String },
			{ "Menu:Keybinds", "TeleportPlayerToCameraToggleKey", Menu::Camera::teleportPlayerToCamera.ToStringVKeyMap(), &Menu::Camera::teleportPlayerToCamera, String },
			{ "Menu:Keybinds", "GoProMode", Menu::Camera::goProMode.ToStringVKeyMap(), &Menu::Camera::goProMode, String },
			{ "Menu:Keybinds", "DisableSafezoneFOVReduction", Menu::Camera::disableSafezoneFOVReduction.ToStringVKeyMap(), &Menu::Camera::disableSafezoneFOVReduction, String },
			{ "Menu:Keybinds", "DisablePhotoModeLimits", Menu::Camera::disablePhotoModeLimits.ToStringVKeyMap(), &Menu::Camera::disablePhotoModeLimits, String },
			{ "Menu:Keybinds", "DisableHeadCorrectionToggleKey", Menu::Camera::disableHeadCorrection.ToStringVKeyMap(), &Menu::Camera::disableHeadCorrection, String },
			{ "Menu:Keybinds", "TeleportToSelectedLocationToggleKey", Menu::Teleport::teleportToSelectedLocation.ToStringVKeyMap(), &Menu::Teleport::teleportToSelectedLocation, String },
			{ "Menu:Keybinds", "TeleportToCoordsToggleKey", Menu::Teleport::teleportToCoords.ToStringVKeyMap(), &Menu::Teleport::teleportToCoords, String },
			{ "Menu:Keybinds", "DisableHUDToggleKey", Menu::Misc::disableHUD.ToStringVKeyMap(), &Menu::Misc::disableHUD, String },
			{ "Menu:Keybinds", "DisableTAAToggleKey", Menu::Misc::disableTAA.ToStringVKeyMap(), &Menu::Misc::disableTAA, String },
			{ "Menu:Keybinds", "DisableGamePauseWhileAFKToggleKey", Menu::Misc::disableGamePauseWhileAFK.ToStringVKeyMap(), &Menu::Misc::disableGamePauseWhileAFK, String },
			{ "Menu:Keybinds", "FreezeTimeToggleKey", Menu::World::freezeTime.ToStringVKeyMap(), &Menu::World::freezeTime, String },
			{ "Menu:Keybinds", "SlowMotionToggleKey", Menu::World::slowMotion.ToStringVKeyMap(), &Menu::World::slowMotion, String },
			{ "Player:Misc", "GodMode", Menu::Player::godMode.GetValue(), &Menu::Player::godMode, OPTION },
			{ "Player:Misc", "UnlimitedImmunity", Menu::Player::unlimitedImmunity.GetValue(), &Menu::Player::unlimitedImmunity, OPTION },
			{ "Player:Misc", "UnlimitedStamina", Menu::Player::unlimitedStamina.GetValue(), &Menu::Player::unlimitedStamina, OPTION },
			{ "Player:Misc", "UnlimitedItems", Menu::Player::unlimitedItems.GetValue(), &Menu::Player::unlimitedItems, OPTION },
			{ "Player:Misc", "OneHitKill", Menu::Player::oneHitKill.GetValue(), &Menu::Player::oneHitKill, OPTION },
			{ "Player:Misc", "InvisibleToEnemies", Menu::Player::invisibleToEnemies.GetValue(), &Menu::Player::invisibleToEnemies, OPTION },
			{ "Player:Misc", "DisableOutOfBoundsTimer", Menu::Player::disableOutOfBoundsTimer.GetValue(), &Menu::Player::disableOutOfBoundsTimer, OPTION },
			{ "Player:Misc", "NightrunnerMode", Menu::Player::nightrunnerMode.GetValue(), &Menu::Player::nightrunnerMode, OPTION },
			{ "Player:Misc", "OneHandedMode", Menu::Player::oneHandedMode.GetValue(), &Menu::Player::oneHandedMode, OPTION },
			{ "Player:Misc", "DisableSafezoneRestrictions", Menu::Player::disableSafezoneRestrictions.GetValue(), &Menu::Player::disableSafezoneRestrictions, OPTION },
			{ "Player:PlayerJumpParameters", "DisableAirControl", Menu::Player::disableAirControl.GetValue(), &Menu::Player::disableAirControl, OPTION },
			{ "Player:PlayerVariables", "Enabled", Menu::Player::playerVariables.GetValue(), &Menu::Player::playerVariables, OPTION },
			{ "Player:PlayerVariables", "LastSaveSCRPath", Menu::Player::saveSCRPath, &Menu::Player::saveSCRPath, String },
			{ "Player:PlayerVariables", "LastLoadSCRFilePath", Menu::Player::loadSCRFilePath, &Menu::Player::loadSCRFilePath, String },
			{ "Weapon:Misc", "UnlimitedDurability", Menu::Weapon::unlimitedDurability.GetValue(), &Menu::Weapon::unlimitedDurability, OPTION },
			{ "Weapon:Misc", "UnlimitedAmmo", Menu::Weapon::unlimitedAmmo.GetValue(), &Menu::Weapon::unlimitedAmmo, OPTION },
			{ "Weapon:Misc", "NoSpread", Menu::Weapon::noSpread.GetValue(), &Menu::Weapon::noSpread, OPTION },
			{ "Weapon:Misc", "NoRecoil", Menu::Weapon::noRecoil.GetValue(), &Menu::Weapon::noRecoil, OPTION },
			{ "Weapon:Misc", "InstantReload", Menu::Weapon::instantReload.GetValue(), &Menu::Weapon::instantReload, OPTION },
			{ "Camera:FreeCam", "FOV", Menu::Camera::freeCamFOV, &Menu::Camera::freeCamFOV, Float },
			{ "Camera:FreeCam", "Speed", Menu::Camera::freeCamSpeed, &Menu::Camera::freeCamSpeed, Float },
			{ "Camera:FreeCam", "TeleportPlayerToCamera", Menu::Camera::teleportPlayerToCamera.GetValue(), &Menu::Camera::teleportPlayerToCamera, OPTION },
			{ "Camera:FirstPerson", "XOffset", Menu::Camera::cameraOffset.X, &Menu::Camera::cameraOffset.X, Float },
			{ "Camera:FirstPerson", "YOffset", Menu::Camera::cameraOffset.Y, &Menu::Camera::cameraOffset.Y, Float },
			{ "Camera:FirstPerson", "ZOffset", Menu::Camera::cameraOffset.Z, &Menu::Camera::cameraOffset.Z, Float },
			{ "Camera:FirstPerson", "ZoomIn", Menu::Camera::firstPersonZoomIn.GetValue(), &Menu::Camera::firstPersonZoomIn, OPTION},
			{ "Camera:ThirdPerson", "Enabled", Menu::Camera::thirdPersonCamera.GetValue(), &Menu::Camera::thirdPersonCamera, OPTION },
			{ "Camera:ThirdPerson", "UseTPPModel", Menu::Camera::tpUseTPPModel.GetValue(), &Menu::Camera::tpUseTPPModel, OPTION },
			{ "Camera:ThirdPerson", "FOV", Menu::Camera::thirdPersonFOV, &Menu::Camera::thirdPersonFOV, Float },
			{ "Camera:ThirdPerson", "DistanceBehindPlayer", Menu::Camera::thirdPersonDistanceBehindPlayer, &Menu::Camera::thirdPersonDistanceBehindPlayer, Float },
			{ "Camera:ThirdPerson", "HeightAbovePlayer", Menu::Camera::thirdPersonHeightAbovePlayer, &Menu::Camera::thirdPersonHeightAbovePlayer, Float },
			{ "Camera:ThirdPerson", "HorizontalDistanceFromPlayer", Menu::Camera::thirdPersonHorizontalDistanceFromPlayer, &Menu::Camera::thirdPersonHorizontalDistanceFromPlayer, Float },
			{ "Camera:Misc", "LensDistortion", Menu::Camera::lensDistortion, &Menu::Camera::lensDistortion, Float },
			{ "Camera:Misc", "GoProMode", Menu::Camera::goProMode.GetValue(), &Menu::Camera::goProMode, OPTION },
			{ "Camera:Misc", "DisableSafezoneFOVReduction", Menu::Camera::disableSafezoneFOVReduction.GetValue(), &Menu::Camera::disableSafezoneFOVReduction, OPTION },
			{ "Camera:Misc", "DisablePhotoModeLimits", Menu::Camera::disablePhotoModeLimits.GetValue(), &Menu::Camera::disablePhotoModeLimits, OPTION },
			{ "Camera:Misc", "DisableHeadCorrection", Menu::Camera::disableHeadCorrection.GetValue(), &Menu::Camera::disableHeadCorrection, OPTION },
			{ "Teleport:SavedLocations", "SavedTeleportLocations", Menu::Teleport::savedTeleportLocationsStr, &Menu::Teleport::savedTeleportLocations, String },
			{ "Misc:Misc", "DisableGamePauseWhileAFK", Menu::Misc::disableGamePauseWhileAFK.GetValue(), &Menu::Misc::disableGamePauseWhileAFK, OPTION },
			{ "Misc:Misc", "DisableTAA", Menu::Misc::disableTAA.GetValue(), &Menu::Misc::disableTAA, OPTION },
			{ "Misc:GameChecks", "DisableSavegameCRCCheck", Menu::Misc::disableSavegameCRCCheck.GetValue(), &Menu::Misc::disableSavegameCRCCheck, OPTION },
			{ "Misc:GameChecks", "DisableDataPAKsCRCCheck", Menu::Misc::disableDataPAKsCRCCheck.GetValue(), &Menu::Misc::disableDataPAKsCRCCheck, OPTION },
			{ "Misc:GameChecks", "IncreaseDataPAKsLimit", Menu::Misc::increaseDataPAKsLimit.GetValue(), &Menu::Misc::increaseDataPAKsLimit, OPTION },
			{ "World:Time", "SlowMotionSpeed", Menu::World::slowMotionSpeed, &Menu::World::slowMotionSpeed, Float },
			{ "World:Time", "SlowMotionTransitionTime", Menu::World::slowMotionTransitionTime, &Menu::World::slowMotionTransitionTime, Float },
			{ "Debug:Misc", "DisableVftableScanning", Menu::Debug::disableVftableScanning.GetValue(), &Menu::Debug::disableVftableScanning, OPTION },
			{ "Debug:Misc", "EnableDebuggingConsole", Menu::Debug::enableDebuggingConsole.GetValue(), &Menu::Debug::enableDebuggingConsole, OPTION }
		});
		configVariables = configVariablesDefault;
	}

	static void UpdateEntry(const ConfigEntry& entry) {
		switch (entry.type) {
		case OPTION:
			reader.UpdateEntry(entry.section.data(), entry.key.data(), std::any_cast<bool>(entry.value));
			break;
		case Int:
			reader.UpdateEntry(entry.section.data(), entry.key.data(), std::any_cast<int>(entry.value));
			break;
		case Float:
			reader.UpdateEntry(entry.section.data(), entry.key.data(), std::any_cast<float>(entry.value));
			break;
		case String:
			reader.UpdateEntry(entry.section.data(), entry.key.data(), std::any_cast<std::string>(entry.value));
			break;
		}
	}

	static void LoadDefaultConfig() {
		reader = inih::INIReader();

		const std::filesystem::path desktopPath = EGSDK::Utils::Files::GetDesktopDir();
		std::string desktopPathStr = desktopPath.string();
		if (!desktopPath.empty() && !(std::filesystem::is_directory(desktopPath.parent_path()) && std::filesystem::is_directory(desktopPath)))
			desktopPathStr = {};

		const std::string loadSCRFilePath = desktopPathStr.empty() ? "" : desktopPathStr + "\\player_variables.scr";

		std::string strValue{};
		for (auto entry : configVariablesDefault) {
			switch (entry.type) {
			case OPTION: {
				ImGui::Option* option = reinterpret_cast<ImGui::Option*>(entry.optionPtr);
				if (option->GetChangesAreDisabled())
					break;
				option->SetBothValues(std::any_cast<bool>(entry.value));
				reader.InsertEntry(entry.section.data(), entry.key.data(), std::any_cast<bool>(entry.value));
				break;
			}
			case Int:
				*reinterpret_cast<int*>(entry.optionPtr) = std::any_cast<int>(entry.value);
				reader.InsertEntry(entry.section.data(), entry.key.data(), std::any_cast<int>(entry.value));
				break;
			case Float:
				*reinterpret_cast<float*>(entry.optionPtr) = std::any_cast<float>(entry.value);
				reader.InsertEntry(entry.section.data(), entry.key.data(), std::any_cast<float>(entry.value));
				break;
			case String:
				strValue = std::any_cast<std::string>(entry.value);

				if (entry.key == "LastSaveSCRPath") {
					entry.value = desktopPathStr;
					Menu::Player::saveSCRPath = desktopPathStr;
				} else if (entry.key == "LastLoadSCRFilePath") {
					entry.value = loadSCRFilePath;
					Menu::Player::loadSCRFilePath = loadSCRFilePath;
				} else if (entry.section == "Menu:Keybinds") {
					int keyCode = ImGui::KeyBindOption::ToKeyCodeVKeyMap(strValue);
					if (keyCode != VK_INVALID)
						reinterpret_cast<ImGui::KeyBindOption*>(entry.optionPtr)->ChangeKeyBind(keyCode);
				} else if (entry.key == "SavedTeleportLocations") {
					Menu::Teleport::savedTeleportLocations = Menu::Teleport::ParseTeleportLocations(strValue);
					Menu::Teleport::UpdateTeleportLocationVisualNames();
				}

				reader.InsertEntry(entry.section.data(), entry.key.data(), strValue);
				break;
			}
		}
	}
	static void LoadAndWriteDefaultConfig() {
		LoadDefaultConfig();
		try {
			inih::INIWriter writer{};
			writer.write(configFileName, reader);
		} catch (const std::runtime_error& e) {
			SPDLOG_ERROR("Error writing file {}: {}", configFileName, e.what());
		}
	}
	static bool ConfigExists() {
		return std::filesystem::exists(configFileName);
	}
	static void CreateConfig() {
		SPDLOG_INFO("{} does not exist (will create now); using default config values", configFileName);
		LoadAndWriteDefaultConfig();
	}
	static void ReadConfig(bool configUpdate = false) {
		try {
			reader = inih::INIReader(configFileName);

			const std::filesystem::path desktopPath = EGSDK::Utils::Files::GetDesktopDir();
			std::string desktopPathStr = desktopPath.string();
			if (!desktopPath.empty() && !(std::filesystem::is_directory(desktopPath.parent_path()) && std::filesystem::is_directory(desktopPath)))
				desktopPathStr = {};

			const std::string loadSCRFilePathStr = desktopPathStr.empty() ? "" : desktopPathStr + "\\player_variables.scr";

			std::string strValue{};
			for (auto& entry : configVariablesDefault) {
				switch (entry.type) {
				case OPTION: {
					ImGui::Option* option = reinterpret_cast<ImGui::Option*>(entry.optionPtr);
					if (option->GetChangesAreDisabled())
						break;

					option->SetBothValues(reader.Get(entry.section.data(), entry.key.data(), std::any_cast<bool>(entry.value)));
					break;
				}
				case Int:
					*reinterpret_cast<int*>(entry.optionPtr) = reader.Get(entry.section.data(), entry.key.data(), std::any_cast<int>(entry.value));
					break;
				case Float:
					*reinterpret_cast<float*>(entry.optionPtr) = reader.Get(entry.section.data(), entry.key.data(), std::any_cast<float>(entry.value));
					break;
				case String:
					strValue = reader.Get(entry.section.data(), entry.key.data(), std::any_cast<std::string>(entry.value));
					if (entry.section == "Menu:Keybinds") {
						int keyCode = ImGui::KeyBindOption::ToKeyCodeVKeyMap(strValue);
						if (keyCode != VK_INVALID)
							reinterpret_cast<ImGui::KeyBindOption*>(entry.optionPtr)->ChangeKeyBind(keyCode);
						break;
					} else if (entry.key == "LastSaveSCRPath") {
						Menu::Player::saveSCRPath = strValue;
						if (Menu::Player::saveSCRPath.empty())
							Menu::Player::saveSCRPath = desktopPathStr;

						const std::filesystem::path saveSCRPath = Menu::Player::saveSCRPath;
						if (!saveSCRPath.empty() && !(std::filesystem::is_directory(saveSCRPath.parent_path()) && std::filesystem::is_directory(saveSCRPath)))
							Menu::Player::saveSCRPath = {};
						break;
					} else if (entry.key == "LastLoadSCRFilePath") {
						Menu::Player::loadSCRFilePath = strValue;
						if (Menu::Player::loadSCRFilePath.empty())
							Menu::Player::loadSCRFilePath = loadSCRFilePathStr;
						const std::filesystem::path loadSCRFilePath = Menu::Player::loadSCRFilePath;

						if (!loadSCRFilePath.empty() && !std::filesystem::is_directory(loadSCRFilePath.parent_path()))
							Menu::Player::loadSCRFilePath = {};
						break;
					} else if (entry.key == "SavedTeleportLocations") {
						Menu::Teleport::savedTeleportLocations = Menu::Teleport::ParseTeleportLocations(strValue.empty() ? std::any_cast<std::string>(entry.value) : strValue);
						Menu::Teleport::UpdateTeleportLocationVisualNames();
						break;
					}

					*reinterpret_cast<std::string*>(entry.optionPtr) = strValue;
					break;
				}
			}

			SPDLOG_INFO(configUpdate ? "Successfully read updated config!" : "Successfully read config!");
		} catch (const std::runtime_error& e) {
			SPDLOG_ERROR("Error writing file {}; using default config values: {}", configFileName, e.what());
			LoadDefaultConfig();
		}
	}

	static bool CheckForChangesInMem() {
		try {
			reader = inih::INIReader(configFileName);

			std::string strValue{};
			for (auto& entry : configVariablesDefault) {
				switch (entry.type) {
				case OPTION:
					if (reader.Get(entry.section.data(), entry.key.data(), std::any_cast<bool>(entry.value)) != reinterpret_cast<ImGui::Option*>(entry.optionPtr)->GetValue())
						return true;
					break;
				case Int:
					if (reader.Get(entry.section.data(), entry.key.data(), std::any_cast<int>(entry.value)) != *reinterpret_cast<int*>(entry.optionPtr))
						return true;
					break;
				case Float:
					if (!EGSDK::Utils::Values::are_samef(reader.Get(entry.section.data(), entry.key.data(), std::any_cast<float>(entry.value)), *reinterpret_cast<float*>(entry.optionPtr)))
						return true;
					break;
				case String:
					strValue = reader.Get(entry.section.data(), entry.key.data(), std::any_cast<std::string>(entry.value));

					if (entry.section == "Menu:Keybinds") {
						int configKeyCode = ImGui::KeyBindOption::ToKeyCodeVKeyMap(strValue);
						int memKeyCode = reinterpret_cast<ImGui::KeyBindOption*>(entry.optionPtr)->GetKeyBind();
						if (configKeyCode == VK_INVALID)
							break;

						if (configKeyCode != memKeyCode)
							return true;
						break;
					} else if (entry.key == "SavedTeleportLocations") {
						std::string currentTPLocations = Menu::Teleport::ConvertTeleportLocationsToStr(Menu::Teleport::savedTeleportLocations);
						if (strValue != currentTPLocations)
							return true;
						break;
					}

					if (strValue != *reinterpret_cast<std::string*>(entry.optionPtr))
						return true;
					break;
				}
			}
			return false;
		} catch (const std::runtime_error& e) {
			SPDLOG_ERROR("Error checking for changes in memory: {}", e.what());
			return false;
		}
	}
	void SaveConfig() {
		for (auto& entry : configVariables) {
			switch (entry.type) {
			case OPTION:
				entry.value = reinterpret_cast<ImGui::Option*>(entry.optionPtr)->GetValue();
				break;
			case Float:
				entry.value = *reinterpret_cast<float*>(entry.optionPtr);
				break;
			case String:
				if (entry.section == "Menu:Keybinds") {
					std::string keyString = reinterpret_cast<ImGui::KeyBindOption*>(entry.optionPtr)->ToStringVKeyMap();
					if (!keyString.empty())
						entry.value = keyString;
					break;
				} else if (entry.key == "SavedTeleportLocations") {
					entry.value = Menu::Teleport::ConvertTeleportLocationsToStr(Menu::Teleport::savedTeleportLocations);
					break;
				}

				entry.value = *reinterpret_cast<std::string*>(entry.optionPtr);
				break;
			}

			UpdateEntry(entry);
		}

		try {
			inih::INIWriter writer{};
			writer.write(configFileName, reader);

			SPDLOG_INFO("Successfully updated config!");

			savedConfig = true;
		} catch (const std::runtime_error& e) {
			SPDLOG_ERROR("Error saving to file {}: {}", configFileName, e.what());
		}
	}
	void InitConfig() {
		InitializeConfigVariables();
		ConfigExists() ? ReadConfig() : CreateConfig();

		configPreviousWriteTime = std::filesystem::last_write_time(configFileName);
		configLastWriteTime = configPreviousWriteTime;
	}
	void ConfigLoop() {
		while (!Core::exiting) {
			Sleep(2000);

			if (!ConfigExists()) {
				CreateConfig();
				Sleep(750);
				configPreviousWriteTime = std::filesystem::last_write_time(configFileName);
				continue;
			}

			// Check for config changes
			configLastWriteTime = std::filesystem::last_write_time(configFileName);
			if (configLastWriteTime != configPreviousWriteTime && !savedConfig) {
				configPreviousWriteTime = configLastWriteTime;
				ReadConfig(true);
				continue;
			} else if (configLastWriteTime != configPreviousWriteTime && savedConfig) {
				configPreviousWriteTime = configLastWriteTime;
				savedConfig = false;
			}

			if (CheckForChangesInMem())
				SaveConfig();
		}
	}
}