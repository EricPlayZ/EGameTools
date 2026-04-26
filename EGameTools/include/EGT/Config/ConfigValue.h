#pragma once
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace EGT::Config {
	enum class ScalarType {
		Int,
		Float,
		String
	};

	using ScalarValue = std::variant<int, float, std::string>;

	class IConfigScalar {
	public:
		IConfigScalar(std::string_view section, std::string_view key, ScalarType type) : section(section), key(key), type(type) {
			GetInstances()->insert(this);
		}

		virtual ~IConfigScalar() {
			GetInstances()->erase(this);
		}

		IConfigScalar(const IConfigScalar&) = delete;
		IConfigScalar& operator=(const IConfigScalar&) = delete;

		[[nodiscard]] const std::string& GetSection() const {
			return section;
		}

		[[nodiscard]] const std::string& GetKey() const {
			return key;
		}

		[[nodiscard]] ScalarType GetType() const {
			return type;
		}

		[[nodiscard]] virtual ScalarValue GetDefaultValue() const = 0;
		[[nodiscard]] virtual ScalarValue GetCurrentValue() const = 0;
		virtual void SetCurrentValue(const ScalarValue& value) = 0;

		[[nodiscard]] static std::set<IConfigScalar*>* GetInstances() {
			static std::set<IConfigScalar*> instances{};
			return &instances;
		}

	private:
		std::string section{};
		std::string key{};
		ScalarType type{ ScalarType::Int };
	};

	template <typename T, ScalarType TypeValue>
	class ConfigScalar : public IConfigScalar {
	public:
		ConfigScalar(std::string_view section, std::string_view key, T defaultValue) : IConfigScalar(section, key, TypeValue), value(defaultValue), defaultValue(defaultValue) {}

		ConfigScalar(const ConfigScalar&) = delete;
		ConfigScalar& operator=(const ConfigScalar&) = delete;

		ConfigScalar& operator=(const T& newValue) {
			value = newValue;
			return *this;
		}

		operator T&() {
			return value;
		}

		operator const T&() const {
			return value;
		}

		T* operator&() {
			return &value;
		}

		const T* operator&() const {
			return &value;
		}

		[[nodiscard]] T& GetValue() {
			return value;
		}

		[[nodiscard]] const T& GetValue() const {
			return value;
		}

		[[nodiscard]] ScalarValue GetDefaultValue() const override {
			return defaultValue;
		}

		[[nodiscard]] ScalarValue GetCurrentValue() const override {
			return value;
		}

		void SetCurrentValue(const ScalarValue& newValue) override {
			value = std::get<T>(newValue);
		}

	private:
		T value{};
		T defaultValue{};
	};

	template <typename T, ScalarType TypeValue>
	class ConfigScalarRef final : public IConfigScalar {
	public:
		ConfigScalarRef(std::string_view section, std::string_view key, T& valueRef) : IConfigScalar(section, key, TypeValue), valuePtr(&valueRef), defaultValue(valueRef) {}

		ConfigScalarRef(std::string_view section, std::string_view key, T& valueRef, T explicitDefaultValue)
			: IConfigScalar(section, key, TypeValue), valuePtr(&valueRef), defaultValue(explicitDefaultValue) {}

		ConfigScalarRef(const ConfigScalarRef&) = delete;
		ConfigScalarRef& operator=(const ConfigScalarRef&) = delete;

		[[nodiscard]] ScalarValue GetDefaultValue() const override {
			return defaultValue;
		}

		[[nodiscard]] ScalarValue GetCurrentValue() const override {
			return *valuePtr;
		}

		void SetCurrentValue(const ScalarValue& value) override {
			*valuePtr = std::get<T>(value);
		}

	private:
		T* valuePtr{};
		T defaultValue{};
	};

	class ConfigString final : public ConfigScalar<std::string, ScalarType::String> {
	public:
		using ConfigScalar<std::string, ScalarType::String>::operator=;

		ConfigString(std::string_view section, std::string_view key, std::string defaultValue)
			: ConfigScalar<std::string, ScalarType::String>(section, key, std::move(defaultValue)) {}

		[[nodiscard]] bool empty() const {
			return GetValue().empty();
		}

		[[nodiscard]] const char* c_str() const {
			return GetValue().c_str();
		}
	};

	using ConfigInt = ConfigScalar<int, ScalarType::Int>;
	using ConfigFloat = ConfigScalar<float, ScalarType::Float>;
	using ConfigIntRef = ConfigScalarRef<int, ScalarType::Int>;
	using ConfigFloatRef = ConfigScalarRef<float, ScalarType::Float>;
}
