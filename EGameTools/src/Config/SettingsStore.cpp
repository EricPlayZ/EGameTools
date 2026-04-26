#include <Windows.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_set>
#include <spdlog\spdlog.h>
#include <EGT\Config\TomlDocument.h>
#include <EGT\Config\ConfigValue.h>
#include <ini\ini.h>
#include <EGSDK\Utils\Files.h>
#include <EGT\Config\ConfigPaths.h>
#include <EGT\Config\SettingsStore.h>
#include <EGT\Config\TeleportStore.h>
#include <ImGui\imgui_hotkey.h>

namespace EGT::Config::SettingsStore {
	enum class ValueType {
		Option,
		Int,
		Float,
		String,
		Keybind
	};

		using SettingDefault = std::variant<bool, int, float, std::string>;
		using SettingPtr = std::variant<ImGui::Option*, Config::IConfigScalar*, ImGui::KeyBindOption*>;

		struct ConfigEntry {
			std::string section;
			std::string key;
			ValueType type;
			SettingDefault defaultValue;
			SettingPtr settingPtr;
		};

	struct LoadStats {
		int importedKeys{ 0 };
		int defaultedKeys{ 0 };
		int parseErrors{ 0 };
	};

	constexpr int schemaVersion = 1;
	static std::vector<ConfigEntry> registry{};
	static TomlDocument settingsDocument{};
	static std::string lastSnapshot{};
	static std::string lastMigratedFrom{};
	static const std::filesystem::path settingsFilePath = Paths::GetSettingsFilePath();

	const std::string GetDesktopPathString() {
			const std::filesystem::path desktopPath = EGSDK::Utils::Files::GetDesktopDir();
			std::string desktopPathStr = desktopPath.string();
			if (!desktopPath.empty() && !(std::filesystem::is_directory(desktopPath.parent_path()) && std::filesystem::is_directory(desktopPath)))
				desktopPathStr = {};
			return desktopPathStr;
		}

	const std::string GetDefaultLoadScrPath() {
			const std::string desktopPathStr = GetDesktopPathString();
			return desktopPathStr.empty() ? "" : desktopPathStr + "\\player_variables.scr";
		}

	void EnsureSpecialStringValue(const ConfigEntry& entry, std::string& value) {
			if (entry.key == "LastSaveSCRPath") {
				if (value.empty())
					value = GetDesktopPathString();

				const std::filesystem::path saveSCRPath = value;
				if (!saveSCRPath.empty() && !(std::filesystem::is_directory(saveSCRPath.parent_path()) && std::filesystem::is_directory(saveSCRPath)))
					value = {};
			} else if (entry.key == "LastLoadSCRFilePath") {
				if (value.empty())
					value = GetDefaultLoadScrPath();

				const std::filesystem::path loadSCRPath = value;
				if (!loadSCRPath.empty() && !std::filesystem::is_directory(loadSCRPath.parent_path()))
					value = {};
			}
		}

	ConfigEntry MakeOptionEntry(ImGui::Option& option) {
		return ConfigEntry{ std::string(option.GetConfigSection()), std::string(option.GetConfigKey()), ValueType::Option, option.GetValue(), &option };
	}

	ConfigEntry MakeKeybindEntry(ImGui::KeyBindOption& option) {
		return ConfigEntry{ std::string(option.GetKeyBindConfigSection()), std::string(option.GetKeyBindConfigKey()), ValueType::Keybind, option.ToStringVKeyMap(), &option };
	}

	ValueType ScalarTypeToValueType(const Config::ScalarType type) {
		switch (type) {
		case Config::ScalarType::Int:
			return ValueType::Int;
		case Config::ScalarType::Float:
			return ValueType::Float;
		case Config::ScalarType::String:
			return ValueType::String;
		}

		return ValueType::String;
	}

	SettingDefault ToSettingDefault(const Config::IConfigScalar& scalar) {
		const Config::ScalarValue scalarDefault = scalar.GetDefaultValue();
		switch (scalar.GetType()) {
		case Config::ScalarType::Int:
			return std::get<int>(scalarDefault);
		case Config::ScalarType::Float:
			return std::get<float>(scalarDefault);
		case Config::ScalarType::String:
			return std::get<std::string>(scalarDefault);
		}

		return std::string{};
	}

	ConfigEntry MakeScalarEntry(Config::IConfigScalar& scalar) {
		return ConfigEntry{ scalar.GetSection(), scalar.GetKey(), ScalarTypeToValueType(scalar.GetType()), ToSettingDefault(scalar), &scalar };
	}

	bool TryRegisterEntry(const ConfigEntry& entry, std::unordered_set<std::string>& seenKeys) {
		std::string fullKey = entry.section + "\n" + entry.key;
		if (!seenKeys.insert(fullKey).second) {
			SPDLOG_ERROR("Duplicate setting key detected: [{}] {}", entry.section, entry.key);
			return false;
		}

		registry.push_back(entry);
		return true;
	}

	void BuildRegistry() {
		if (!registry.empty())
			return;

		std::unordered_set<std::string> seenKeys{};
		seenKeys.reserve(256);
		std::unordered_set<ImGui::Option*> keybindAsOptionPointers{};
		for (auto* keybind : *ImGui::KeyBindOption::GetInstances())
			keybindAsOptionPointers.insert(static_cast<ImGui::Option*>(keybind));

		for (auto* option : *ImGui::Option::GetInstances()) {
			if (!option->HasConfigBinding())
				continue;
			if (keybindAsOptionPointers.find(option) != keybindAsOptionPointers.end())
				continue;

			TryRegisterEntry(MakeOptionEntry(*option), seenKeys);
		}

		for (auto* keybind : *ImGui::KeyBindOption::GetInstances()) {
			if (keybind->HasConfigBinding())
				TryRegisterEntry(MakeOptionEntry(*keybind), seenKeys);
			if (keybind->HasKeyBindConfigBinding())
				TryRegisterEntry(MakeKeybindEntry(*keybind), seenKeys);
		}

		for (auto* scalar : *Config::IConfigScalar::GetInstances())
			TryRegisterEntry(MakeScalarEntry(*scalar), seenKeys);

		SPDLOG_INFO("Settings registry discovered {} entries", registry.size());
	}

	bool ReaderContainsKey(const TomlDocument& reader, const ConfigEntry& entry) {
		return reader.Has(entry.section, entry.key);
	}

	bool ReaderContainsLegacyKey(inih::INIReader& reader, const ConfigEntry& entry) {
		try {
			reader.Get<std::string>(std::string(entry.section), std::string(entry.key));
			return true;
		} catch (...) {
			return false;
		}
	}

	std::string GetDefaultStringForEntry(const ConfigEntry& entry) {
			if (entry.key == "LastSaveSCRPath")
				return GetDesktopPathString();
			if (entry.key == "LastLoadSCRFilePath")
				return GetDefaultLoadScrPath();
			return std::get<std::string>(entry.defaultValue);
		}

	void ApplyBoolOption(ImGui::Option* option, bool value) {
			if (option->GetChangesAreDisabled())
				return;
			option->SetBothValues(value);
		}

	void LoadFromReader(const TomlDocument& reader, LoadStats* stats) {
			for (const auto& entry : registry) {
				switch (entry.type) {
				case ValueType::Option:
				{
					bool value = std::get<bool>(entry.defaultValue);
					if (ReaderContainsKey(reader, entry)) {
						try {
							value = reader.GetBool(entry.section, entry.key, value);
							if (stats)
								stats->importedKeys++;
						} catch (...) {
							if (stats)
								stats->parseErrors++;
						}
					} else if (stats)
						stats->defaultedKeys++;
					ApplyBoolOption(std::get<ImGui::Option*>(entry.settingPtr), value);
					break;
				}
				case ValueType::Int:
				{
					int value = std::get<int>(entry.defaultValue);
					if (ReaderContainsKey(reader, entry)) {
						try {
							value = reader.GetInt(entry.section, entry.key, value);
							if (stats)
								stats->importedKeys++;
						} catch (...) {
							if (stats)
								stats->parseErrors++;
						}
					} else if (stats)
						stats->defaultedKeys++;
					std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
					break;
				}
				case ValueType::Float:
				{
					float value = std::get<float>(entry.defaultValue);
					if (ReaderContainsKey(reader, entry)) {
						try {
							value = reader.GetFloat(entry.section, entry.key, value);
							if (stats)
								stats->importedKeys++;
						} catch (...) {
							if (stats)
								stats->parseErrors++;
						}
					} else if (stats)
						stats->defaultedKeys++;
					std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
					break;
				}
				case ValueType::String:
				{
					std::string value = GetDefaultStringForEntry(entry);
					if (ReaderContainsKey(reader, entry)) {
						try {
							value = reader.GetString(entry.section, entry.key, value);
							if (stats)
								stats->importedKeys++;
						} catch (...) {
							if (stats)
								stats->parseErrors++;
						}
					} else if (stats)
						stats->defaultedKeys++;

					EnsureSpecialStringValue(entry, value);
					std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
					break;
				}
				case ValueType::Keybind:
				{
					const std::string defaultKey = std::get<std::string>(entry.defaultValue);
					std::string value = defaultKey;
					if (ReaderContainsKey(reader, entry)) {
						try {
							value = reader.GetString(entry.section, entry.key, defaultKey);
							if (stats)
								stats->importedKeys++;
						} catch (...) {
							if (stats)
								stats->parseErrors++;
						}
					} else if (stats)
						stats->defaultedKeys++;

					if (value.empty())
						break;

					int keyCode = ImGui::KeyBindOption::ToKeyCodeVKeyMap(value);
					if (keyCode != VK_INVALID)
						std::get<ImGui::KeyBindOption*>(entry.settingPtr)->ChangeKeyBind(keyCode);
					else if (stats)
						stats->parseErrors++;
					break;
				}
				}
			}
	}

	void LoadFromLegacyIni(inih::INIReader& reader, LoadStats* stats) {
		for (const auto& entry : registry) {
			switch (entry.type) {
			case ValueType::Option:
			{
				bool value = std::get<bool>(entry.defaultValue);
				if (ReaderContainsLegacyKey(reader, entry)) {
					try {
						value = reader.Get(std::string(entry.section), std::string(entry.key), static_cast<bool>(value));
						if (stats)
							stats->importedKeys++;
					} catch (...) {
						if (stats)
							stats->parseErrors++;
					}
				} else if (stats) {
					stats->defaultedKeys++;
				}
				ApplyBoolOption(std::get<ImGui::Option*>(entry.settingPtr), value);
				break;
			}
			case ValueType::Int:
			{
				int value = std::get<int>(entry.defaultValue);
				if (ReaderContainsLegacyKey(reader, entry)) {
					try {
						value = reader.Get(std::string(entry.section), std::string(entry.key), static_cast<int>(value));
						if (stats)
							stats->importedKeys++;
					} catch (...) {
						if (stats)
							stats->parseErrors++;
					}
				} else if (stats) {
					stats->defaultedKeys++;
				}
				std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
				break;
			}
			case ValueType::Float:
			{
				float value = std::get<float>(entry.defaultValue);
				if (ReaderContainsLegacyKey(reader, entry)) {
					try {
						value = reader.Get(std::string(entry.section), std::string(entry.key), static_cast<float>(value));
						if (stats)
							stats->importedKeys++;
					} catch (...) {
						if (stats)
							stats->parseErrors++;
					}
				} else if (stats) {
					stats->defaultedKeys++;
				}
				std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
				break;
			}
			case ValueType::String:
			{
				std::string value = GetDefaultStringForEntry(entry);
				if (ReaderContainsLegacyKey(reader, entry)) {
					try {
						value = reader.Get(std::string(entry.section), std::string(entry.key), std::string{ value });
						if (stats)
							stats->importedKeys++;
					} catch (...) {
						if (stats)
							stats->parseErrors++;
					}
				} else if (stats) {
					stats->defaultedKeys++;
				}

				EnsureSpecialStringValue(entry, value);
				std::get<Config::IConfigScalar*>(entry.settingPtr)->SetCurrentValue(value);
				break;
			}
			case ValueType::Keybind:
			{
				const std::string defaultKey = std::get<std::string>(entry.defaultValue);
				std::string value = defaultKey;
				if (ReaderContainsLegacyKey(reader, entry)) {
					try {
						value = reader.Get(std::string(entry.section), std::string(entry.key), std::string{ defaultKey });
						if (stats)
							stats->importedKeys++;
					} catch (...) {
						if (stats)
							stats->parseErrors++;
					}
				} else if (stats) {
					stats->defaultedKeys++;
				}

				if (value.empty())
					break;

				int keyCode = ImGui::KeyBindOption::ToKeyCodeVKeyMap(value);
				if (keyCode != VK_INVALID)
					std::get<ImGui::KeyBindOption*>(entry.settingPtr)->ChangeKeyBind(keyCode);
				else if (stats)
					stats->parseErrors++;
				break;
			}
			}
		}
	}

	void SyncDocumentWithCurrentValues() {
			settingsDocument.Clear();
			settingsDocument.SetInt("meta", "schemaVersion", schemaVersion);
			settingsDocument.SetString("meta", "lastMigratedFrom", lastMigratedFrom.empty() ? "none" : lastMigratedFrom);

			for (const auto& entry : registry) {
				switch (entry.type) {
				case ValueType::Option:
					settingsDocument.SetBool(entry.section, entry.key, std::get<ImGui::Option*>(entry.settingPtr)->GetValue());
					break;
				case ValueType::Int:
					settingsDocument.SetInt(entry.section, entry.key, std::get<int>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue()));
					break;
				case ValueType::Float:
					settingsDocument.SetFloat(entry.section, entry.key, std::get<float>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue()));
					break;
				case ValueType::String:
					settingsDocument.SetString(entry.section, entry.key, std::get<std::string>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue()));
					break;
				case ValueType::Keybind:
					settingsDocument.SetString(entry.section, entry.key, std::get<ImGui::KeyBindOption*>(entry.settingPtr)->ToStringVKeyMap());
					break;
				}
			}
		}

	std::string BuildCurrentSnapshot() {
			std::string snapshot{};
			for (const auto& entry : registry) {
				snapshot += std::string(entry.section);
				snapshot += ".";
				snapshot += std::string(entry.key);
				snapshot += "=";

				switch (entry.type) {
				case ValueType::Option:
					snapshot += std::get<ImGui::Option*>(entry.settingPtr)->GetValue() ? "1" : "0";
					break;
				case ValueType::Int:
					snapshot += std::to_string(std::get<int>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue()));
					break;
				case ValueType::Float:
					snapshot += std::format("{:.6f}", std::get<float>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue()));
					break;
				case ValueType::String:
					snapshot += std::get<std::string>(std::get<Config::IConfigScalar*>(entry.settingPtr)->GetCurrentValue());
					break;
				case ValueType::Keybind:
					snapshot += std::get<ImGui::KeyBindOption*>(entry.settingPtr)->ToStringVKeyMap();
					break;
				}
				snapshot.push_back('\n');
			}
			return snapshot;
		}

	void WriteMigrationReport(const LoadStats& stats) {
			try {
				std::ofstream report{ Paths::GetMigrationReportPath(), std::ios::trunc };
				if (!report.is_open())
					return;
				report << "settings migration report\n";
				report << "source=" << Paths::GetLegacyConfigFilePath().string() << '\n';
				report << "destination=" << settingsFilePath.string() << '\n';
				report << "imported_keys=" << stats.importedKeys << '\n';
				report << "defaulted_keys=" << stats.defaultedKeys << '\n';
				report << "parse_errors=" << stats.parseErrors << '\n';
			} catch (...) {
			}
		}

	void EnsureDefaultsApplied() {
		BuildRegistry();
		TomlDocument defaultsReader{};
		LoadStats ignored{};
		LoadFromReader(defaultsReader, &ignored);
		lastSnapshot = BuildCurrentSnapshot();
	}

	bool FileExists() {
		return std::filesystem::exists(settingsFilePath);
	}

	const std::filesystem::path& GetSettingsFilePath() {
		return settingsFilePath;
	}

	bool ReloadFromDisk() {
		BuildRegistry();

		try {
			TomlDocument reader{};
			std::string errorMessage{};
			if (!reader.Load(settingsFilePath, &errorMessage)) {
				SPDLOG_ERROR("Failed loading settings from {}: {}", settingsFilePath.string(), errorMessage);
				return false;
			}
			LoadStats stats{};
			LoadFromReader(reader, &stats);
			SyncDocumentWithCurrentValues();
			lastSnapshot = BuildCurrentSnapshot();
			SPDLOG_INFO("Loaded settings from {}", settingsFilePath.string());
			return true;
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Failed loading settings from {}: {}", settingsFilePath.string(), e.what());
			return false;
		}
	}

	bool Save() {
		BuildRegistry();

		try {
			std::filesystem::create_directories(settingsFilePath.parent_path());
			SyncDocumentWithCurrentValues();
			std::string errorMessage{};
			if (!settingsDocument.Save(settingsFilePath, &errorMessage)) {
				SPDLOG_ERROR("Failed saving settings to {}: {}", settingsFilePath.string(), errorMessage);
				return false;
			}
			lastSnapshot = BuildCurrentSnapshot();
			SPDLOG_INFO("Saved settings to {}", settingsFilePath.string());
			return true;
		} catch (const std::exception& e) {
			SPDLOG_ERROR("Failed saving settings to {}: {}", settingsFilePath.string(), e.what());
			return false;
		}
	}

	bool HasInMemoryChanges() {
		return BuildCurrentSnapshot() != lastSnapshot;
	}

	InitResult Init() {
		BuildRegistry();
		std::filesystem::create_directories(Paths::GetRuntimeDataDir());

		InitResult result{};
		if (FileExists()) {
			result.loadedExistingSettings = ReloadFromDisk();
			if (!result.loadedExistingSettings) {
				EnsureDefaultsApplied();
				Save();
				result.createdFromDefaults = true;
			}
			return result;
		}

		EnsureDefaultsApplied();

		const auto legacyConfigPath = Paths::GetLegacyConfigFilePath();
		if (std::filesystem::exists(legacyConfigPath)) {
			try {
				inih::INIReader legacyReader{ legacyConfigPath.string() };
				LoadStats migrationStats{};
				LoadFromLegacyIni(legacyReader, &migrationStats);

				try {
					const std::string legacyTeleportData = legacyReader.Get<std::string>("Teleport:SavedLocations", "SavedTeleportLocations");
					TeleportStore::ImportSerializedLocations(legacyTeleportData);
				} catch (...) {
				}

				lastMigratedFrom = legacyConfigPath.filename().string();
				Save();

				const std::filesystem::path backupPath = legacyConfigPath.string() + ".bak";
				std::filesystem::copy_file(legacyConfigPath, backupPath, std::filesystem::copy_options::overwrite_existing);

				WriteMigrationReport(migrationStats);

				result.migratedFromLegacy = true;
				return result;
			} catch (const std::exception& e) {
				SPDLOG_ERROR("Failed migrating legacy config {}: {}", legacyConfigPath.string(), e.what());
			}
		}

		Save();
		result.createdFromDefaults = true;
		return result;
	}
}
