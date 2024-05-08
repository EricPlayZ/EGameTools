#include <pch.h>
#include "..\config\config.h"
#include "..\offsets.h"
#include "PlayerState.h"
#include "PlayerVariables.h"

namespace GamePH {
	static const int FLOAT_VAR_OFFSET = 3;
	static const int BOOL_VAR_OFFSET = 2;
	static const int VAR_LOC_OFFSET = 1;

	std::vector<std::pair<std::string, std::pair<LPVOID, std::string>>> PlayerVariables::playerVars;
	std::vector<std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerVarsDefault;
	std::vector<std::pair<std::string, std::pair<std::any, std::string>>> PlayerVariables::playerCustomVarsDefault;
	bool PlayerVariables::gotPlayerVars = false;

	template <typename T>
	static void updateDefaultVar(std::vector<std::pair<std::string, std::pair<std::any, std::string>>>& defaultVars, const std::string& varName, T varValue) {
		static_assert(std::is_same<T, float>::value || std::is_same<T, bool>::value, "Invalid type: value must be float or bool");

		auto it = std::find_if(defaultVars.begin(), defaultVars.end(), [&varName](const auto& pair) {
			return pair.first == varName;
		});
		if (it == defaultVars.end())
			return;

		it->second.first.template emplace<T>(varValue);
	}
	static void processPlayerVar(PDWORD64*& playerVarsMem, std::pair<std::string, std::pair<LPVOID, std::string>>& var) {
		while (true) {
			const bool isFloatPlayerVar = *playerVarsMem == Offsets::GetVT_FloatPlayerVariable();
			const bool isBoolPlayerVar = *playerVarsMem == Offsets::GetVT_BoolPlayerVariable();

			if (isFloatPlayerVar || isBoolPlayerVar) {
				var.second.first = playerVarsMem + VAR_LOC_OFFSET;
				const std::string& varName = var.first;

				if (isFloatPlayerVar) {
					float* varValue = reinterpret_cast<float*>(var.second.first);
					updateDefaultVar(GamePH::PlayerVariables::playerVarsDefault, varName, *varValue);
					updateDefaultVar(GamePH::PlayerVariables::playerCustomVarsDefault, varName, *varValue);

					playerVarsMem += FLOAT_VAR_OFFSET;
				} else {
					bool* varValue = reinterpret_cast<bool*>(var.second.first);
					updateDefaultVar(GamePH::PlayerVariables::playerVarsDefault, varName, *varValue);
					updateDefaultVar(GamePH::PlayerVariables::playerCustomVarsDefault, varName, *varValue);

					playerVarsMem += BOOL_VAR_OFFSET;
				}

				break;
			} else
				playerVarsMem += 1;
		}
	}

	void PlayerVariables::GetPlayerVars() {
		if (gotPlayerVars)
			return;
		if (!Get())
			return;
		if (playerVars.empty())
			return;
		if (!Offsets::GetVT_FloatPlayerVariable())
			return;
		if (!Offsets::GetVT_BoolPlayerVariable())
			return;

		PDWORD64* playerVarsMem = reinterpret_cast<PDWORD64*>(Get());

		for (auto& var : playerVars)
			processPlayerVar(playerVarsMem, var);

		gotPlayerVars = true;
	}
	void PlayerVariables::SortPlayerVars() {
		if (!playerVars.empty())
			return;

		std::stringstream ss(Config::playerVars);

		while (ss.good()) {
			// separate the string by the , character to get each variable
			std::string pVar{};
			getline(ss, pVar, ',');

			std::stringstream ssPVar(pVar);

			std::string varName{};
			std::string varType{};

			while (ssPVar.good()) {
				// seperate the string by the : character to get name and type of variable
				std::string subStr{};
				getline(ssPVar, subStr, ':');

				if (subStr != "float" && subStr != "bool")
					varName = subStr;
				else
					varType = subStr;
			}

			PlayerVariables::playerVars.emplace_back(varName, std::make_pair(nullptr, varType));
			PlayerVariables::playerVarsDefault.emplace_back(varName, std::make_pair(varType == "float" ? 0.0f : false, varType));
			PlayerVariables::playerCustomVarsDefault.emplace_back(varName, std::make_pair(varType == "float" ? 0.0f : false, varType));
		}
	}

	PlayerVariables* PlayerVariables::Get() {
		__try {
			PlayerState* pPlayerState = PlayerState::Get();
			if (!pPlayerState)
				return nullptr;

			PlayerVariables* ptr = pPlayerState->playerVars;
			if (!Utils::Memory::IsValidPtrMod(ptr, "gamedll_ph_x64_rwdi.dll"))
				return nullptr;

			return ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
}