#pragma once
#include <WTypesbase.h>
#include <any>
#include <vector>
#include "..\buffer.h"
#include "..\core.h"
#include "..\utils\values.h"

namespace GamePH {
	class PlayerVariables {
	public:
		static std::vector<std::pair<std::string, std::pair<LPVOID, std::string>>> playerVars;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerVarsDefault;
		static std::vector<std::pair<std::string, std::pair<std::any, std::string>>> playerCustomVarsDefault;
		static bool gotPlayerVars;

		static void GetPlayerVars();
		static void SortPlayerVars();

		template <typename T> static T GetPlayerVar(const std::string& playerVar) {
			static_assert(std::is_same<T, bool>::value || std::is_same<T, float>::value || std::is_same<T, std::string>::value, "Invalid type: value must be bool, float or string");

			auto it = std::find_if(PlayerVariables::playerVars.begin(), PlayerVariables::playerVars.end(), [&playerVar](const auto& pair) {
				return pair.first == playerVar;
			});

			if (it == PlayerVariables::playerVars.end()) {
				if (std::is_same<T, std::string>::value)
					return {};
				else if (std::is_same<T, float>::value)
					return -404.0f;
				else
					return false;
			}

			return *reinterpret_cast<T*>(it->second.first);
		}
		template <typename T> static void ChangePlayerVar(const std::string& playerVar, const T value) {
			static_assert(std::is_same<T, bool>::value || std::is_same<T, float>::value || std::is_same<T, std::string>::value, "Invalid type: value must be bool, float or string");

			if (!gotPlayerVars)
				return;

			auto it = std::find_if(PlayerVariables::playerVars.begin(), PlayerVariables::playerVars.end(), [&playerVar](const auto& pair) {
				return pair.first == playerVar;
			});

			if (it == PlayerVariables::playerVars.end())
				return;

			if (std::is_same<T, std::string>::value) {
				std::string valueStr = Utils::Values::to_string(value);
				if (it->second.second == "float") {
					float* const varValue = reinterpret_cast<float*>(it->second.first);
					const float actualValue = std::stof(valueStr);

					*varValue = actualValue;
					*(varValue + 1) = actualValue;
				} else {
					bool* const varValue = reinterpret_cast<bool*>(it->second.first);
					const bool actualValue = valueStr == "true";

					*varValue = actualValue;
					*(varValue + 1) = actualValue;
				}
			} else {
				T* const varValue = reinterpret_cast<T*>(it->second.first);

				*varValue = value;
				*(varValue + 1) = value;
			}
		}
		template <typename T> static void ManagePlayerVarOption(const std::string& playerVar, const T valueIfTrue, const T valueIfFalse, Option* option, const bool& usePreviousVal = true) {
			if (!gotPlayerVars)
				return;

			static T previousPlayerVarValue = GamePH::PlayerVariables::GetPlayerVar<T>(playerVar);

			if (option->GetValue()) {
				if (!option->GetPrevValue())
					previousPlayerVarValue = GamePH::PlayerVariables::GetPlayerVar<T>(playerVar);

				GamePH::PlayerVariables::ChangePlayerVar(playerVar, valueIfTrue);
				option->SetPrevValue(true);
			} else if (option->GetPrevValue()) {
				option->SetPrevValue(false);
				GamePH::PlayerVariables::ChangePlayerVar(playerVar, usePreviousVal ? previousPlayerVarValue : valueIfFalse);
			}
		}

		static PlayerVariables* Get();
	};
}