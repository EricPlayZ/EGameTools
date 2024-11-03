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
	static bool sortedPlayerVars = false;

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
		if (!sortedPlayerVars)
			return;
		if (!Get())
			return;
		if (!Offsets::GetVT_FloatPlayerVariable() || !Offsets::GetVT_BoolPlayerVariable())
			return;

		DWORD64** playerVarsMem = reinterpret_cast<DWORD64**>(Get());

		for (auto& var : playerVars)
			processPlayerVar(playerVarsMem, var);

		gotPlayerVars = true;
	}

#pragma region Player Variables Sorting
	struct VarTypeFieldMeta {
		PlayerVariables::PlayerVarType type;
		LPVOID(*getFieldMetaVT)();
	};
	const std::vector<VarTypeFieldMeta> varTypeFields = {
		{ PlayerVariables::PlayerVarType::Float, Offsets::GetVT_TypedFieldMetaFloatPlayerVariable },
		{ PlayerVariables::PlayerVarType::Bool, Offsets::GetVT_TypedFieldMetaBoolPlayerVariable }
	};

	bool isRetInstruction(BYTE* address) {
		return address[0] == 0xC3 && address[1] == 0xCC;
	}
	bool isLeaInstruction(BYTE* address, BYTE REX, BYTE ModRM) {
		return address[0] == REX && address[1] == 0x8D && address[2] == ModRM;
	}
	bool isCallInstruction(BYTE* address) {
		return address[0] == 0xE8 && address[4] != 0xE8;
	}
	bool isBelowFuncSizeLimit(BYTE* address, DWORD64 startOfFunc, size_t sizeLimit) {
		return (reinterpret_cast<DWORD64>(address) - startOfFunc) < sizeLimit;
	}

	// to prevent infinite loops, assuming function is no longer than 500000 bytes LMAO Techland... why is your function even like 250000 bytes to begin with? bad code...
	static const size_t MAX_FUNC_SIZE = 500000;
	static const size_t MAX_LOAD_VAR_FUNC_SIZE = 2000;

	static const char* getPlayerVarName(BYTE*& funcAddress, DWORD64 startOfFunc) {
		const char* playerVarName = nullptr;
		while (!playerVarName && !isRetInstruction(funcAddress) && isBelowFuncSizeLimit(funcAddress, startOfFunc, MAX_FUNC_SIZE)) {
			// lea r8, varNameString
			if (!isLeaInstruction(funcAddress, 0x4C, 0x05)) {
				funcAddress++;
				continue;
			}

			playerVarName = reinterpret_cast<const char*>(Utils::Memory::CalcTargetAddrOfRelInst(reinterpret_cast<DWORD64>(funcAddress), 3));
			if (!playerVarName) {
				funcAddress++;
				continue;
			}

			// add the size of the instruction, so we skip this instruction because this instruction is the name
			funcAddress += 0x7;
		}

		return playerVarName;
	}
	PlayerVariables::PlayerVarType getPlayerVarType(BYTE*& funcAddress, DWORD64 startOfFunc) {
		PlayerVariables::PlayerVarType playerVarType = PlayerVariables::PlayerVarType::NONE;

		while (!playerVarType && !isRetInstruction(funcAddress) && isBelowFuncSizeLimit(funcAddress, startOfFunc, MAX_FUNC_SIZE)) {
			// call LoadPlayerXVariable
			if (!isCallInstruction(funcAddress)) {
				funcAddress++;
				continue;
			}

			DWORD64 startOfLoadVarFunc = Utils::Memory::CalcTargetAddrOfRelInst(reinterpret_cast<DWORD64>(funcAddress), 1);
			for (const auto& varType : varTypeFields) {
				DWORD64 metaVTAddr = reinterpret_cast<DWORD64>(varType.getFieldMetaVT());
				BYTE* loadVarFuncAddress = reinterpret_cast<BYTE*>(startOfLoadVarFunc);
				DWORD64 metaVTAddrFromFunc = 0;

				while (!metaVTAddrFromFunc && !isRetInstruction(loadVarFuncAddress) && isBelowFuncSizeLimit(loadVarFuncAddress, startOfLoadVarFunc, MAX_LOAD_VAR_FUNC_SIZE)) {
					// lea rax, typedFieldMetaVT
					if (!isLeaInstruction(loadVarFuncAddress, 0x48, 0x05)) {
						loadVarFuncAddress++;
						continue;
					}

					metaVTAddrFromFunc = Utils::Memory::CalcTargetAddrOfRelInst(reinterpret_cast<DWORD64>(loadVarFuncAddress), 3);
					if (metaVTAddrFromFunc != metaVTAddr) {
						metaVTAddrFromFunc = 0;
						loadVarFuncAddress++;
						continue;
					}
				}

				if (metaVTAddr == metaVTAddrFromFunc) {
					playerVarType = varType.type;
					break;
				}
			}

			// if it's still NONE after seeing the function doesnt reference any of the variables, break so the loop stops
			if (playerVarType == PlayerVariables::PlayerVarType::NONE)
				break;
		}

		return playerVarType;
	}

	void PlayerVariables::SortPlayerVars() {
		DWORD64 startOfFunc = 0;
		while (!startOfFunc)
			startOfFunc = reinterpret_cast<DWORD64>(Offsets::Get_LoadPlayerVars());

		BYTE* funcAddress = reinterpret_cast<BYTE*>(startOfFunc);
		while (!isRetInstruction(funcAddress) && (reinterpret_cast<DWORD64>(funcAddress) - startOfFunc) < MAX_FUNC_SIZE) {
			const char* playerVarName = getPlayerVarName(funcAddress, startOfFunc);
			if (!playerVarName)
				continue;

			PlayerVarType playerVarType = getPlayerVarType(funcAddress, startOfFunc);
			if (!playerVarType)
				continue;

			std::string varType{};
			switch (playerVarType) {
			case PlayerVarType::Float:
				varType = "float";
				break;
			case PlayerVarType::Bool:
				varType = "bool";
				break;
			default:
				break;
			}

			PlayerVariables::playerVars.emplace_back(playerVarName, std::make_pair(nullptr, varType));
			PlayerVariables::playerVarsDefault.emplace_back(playerVarName, std::make_pair(varType == "float" ? 0.0f : false, varType));
			PlayerVariables::playerCustomVarsDefault.emplace_back(playerVarName, std::make_pair(varType == "float" ? 0.0f : false, varType));
		}

		sortedPlayerVars = true;
	}
#pragma endregion

	std::unordered_map<std::string, std::any> PlayerVariables::prevPlayerVarValueMap{};
	std::unordered_map<std::string, bool> PlayerVariables::prevOptionValueMap{};

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