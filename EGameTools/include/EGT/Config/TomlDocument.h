#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <tomlplusplus\toml.hpp>

namespace EGT::Config {
	class TomlDocument {
	public:
		bool Load(const std::filesystem::path& path, std::string* errorMessage = nullptr);
		bool Save(const std::filesystem::path& path, std::string* errorMessage = nullptr) const;

		void Clear();
		bool Has(std::string_view section, std::string_view key) const;

		std::string GetString(std::string_view section, std::string_view key, std::string_view defaultValue) const;
		bool GetBool(std::string_view section, std::string_view key, bool defaultValue) const;
		int GetInt(std::string_view section, std::string_view key, int defaultValue) const;
		float GetFloat(std::string_view section, std::string_view key, float defaultValue) const;

		void SetString(std::string_view section, std::string_view key, std::string_view value);
		void SetBool(std::string_view section, std::string_view key, bool value);
		void SetInt(std::string_view section, std::string_view key, int value);
		void SetFloat(std::string_view section, std::string_view key, float value);

	private:
		toml::table values{};
		toml::table& GetOrCreateSection(std::string_view section);
		const toml::node* GetNode(std::string_view section, std::string_view key) const;
	};
}
