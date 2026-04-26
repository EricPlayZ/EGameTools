#include <fstream>
#include <iomanip>
#include <sstream>
#include <EGT\Config\TomlDocument.h>

namespace EGT::Config {
	toml::table& TomlDocument::GetOrCreateSection(std::string_view section) {
		const std::string sectionName{ section };
		toml::node_view<toml::node> sectionNode = values[sectionName];
		if (!sectionNode || !sectionNode.is_table())
			values.insert_or_assign(sectionName, toml::table{});
		return *values[sectionName].as_table();
	}

	const toml::node* TomlDocument::GetNode(std::string_view section, std::string_view key) const {
		const toml::node_view<const toml::node> sectionNode = values[std::string(section)];
		const toml::table* sectionTable = sectionNode.as_table();
		if (!sectionTable)
			return nullptr;

		const toml::node_view<const toml::node> keyNode = (*sectionTable)[std::string(key)];
		return keyNode.node();
	}

	bool TomlDocument::Load(const std::filesystem::path& path, std::string* errorMessage) {
		try {
			values = toml::parse_file(path.string());
			if (errorMessage)
				errorMessage->clear();
			return true;
		} catch (const toml::parse_error& error) {
			if (errorMessage)
				*errorMessage = error.description();
			return false;
		} catch (const std::exception& error) {
			if (errorMessage)
				*errorMessage = error.what();
			return false;
		}
	}

	bool TomlDocument::Save(const std::filesystem::path& path, std::string* errorMessage) const {
		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			if (errorMessage)
				*errorMessage = "failed opening TOML output file";
			return false;
		}

		file << values;

		if (errorMessage)
			errorMessage->clear();
		return true;
	}

	void TomlDocument::Clear() {
		values.clear();
	}

	bool TomlDocument::Has(std::string_view section, std::string_view key) const {
		return GetNode(section, key) != nullptr;
	}

	std::string TomlDocument::GetString(std::string_view section, std::string_view key, std::string_view defaultValue) const {
		const toml::node* node = GetNode(section, key);
		if (!node)
			return std::string(defaultValue);

		if (const auto stringValue = node->value<std::string>())
			return *stringValue;
		if (const auto boolValue = node->value<bool>())
			return *boolValue ? "true" : "false";
		if (const auto intValue = node->value<int64_t>())
			return std::to_string(*intValue);
		if (const auto floatValue = node->value<double>()) {
			std::ostringstream stream{};
			stream << std::setprecision(6) << *floatValue;
			return stream.str();
		}

		return std::string(defaultValue);
	}

	bool TomlDocument::GetBool(std::string_view section, std::string_view key, bool defaultValue) const {
		const toml::node* node = GetNode(section, key);
		if (!node)
			return defaultValue;
		if (const auto boolValue = node->value<bool>())
			return *boolValue;
		if (const auto intValue = node->value<int64_t>())
			return *intValue != 0;

		const std::string value = GetString(section, key, defaultValue ? "true" : "false");
		if (value == "true" || value == "1" || value == "yes" || value == "on")
			return true;
		if (value == "false" || value == "0" || value == "no" || value == "off")
			return false;
		return defaultValue;
	}

	int TomlDocument::GetInt(std::string_view section, std::string_view key, int defaultValue) const {
		const toml::node* node = GetNode(section, key);
		if (!node)
			return defaultValue;
		if (const auto intValue = node->value<int64_t>())
			return static_cast<int>(*intValue);
		if (const auto floatValue = node->value<double>())
			return static_cast<int>(*floatValue);
		return defaultValue;
	}

	float TomlDocument::GetFloat(std::string_view section, std::string_view key, float defaultValue) const {
		const toml::node* node = GetNode(section, key);
		if (!node)
			return defaultValue;
		if (const auto floatValue = node->value<double>())
			return static_cast<float>(*floatValue);
		if (const auto intValue = node->value<int64_t>())
			return static_cast<float>(*intValue);
		return defaultValue;
	}

	void TomlDocument::SetString(std::string_view section, std::string_view key, std::string_view value) {
		toml::table& sectionTable = GetOrCreateSection(section);
		sectionTable.insert_or_assign(std::string(key), std::string(value));
	}

	void TomlDocument::SetBool(std::string_view section, std::string_view key, bool value) {
		toml::table& sectionTable = GetOrCreateSection(section);
		sectionTable.insert_or_assign(std::string(key), value);
	}

	void TomlDocument::SetInt(std::string_view section, std::string_view key, int value) {
		toml::table& sectionTable = GetOrCreateSection(section);
		sectionTable.insert_or_assign(std::string(key), static_cast<int64_t>(value));
	}

	void TomlDocument::SetFloat(std::string_view section, std::string_view key, float value) {
		toml::table& sectionTable = GetOrCreateSection(section);
		sectionTable.insert_or_assign(std::string(key), static_cast<double>(value));
	}
}
