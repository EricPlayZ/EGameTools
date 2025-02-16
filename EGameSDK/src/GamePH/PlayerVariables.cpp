#include <algorithm>
#include <mutex>
#include <spdlog\spdlog.h>
#include <EGSDK\Offsets.h>
#include <EGSDK\Utils\Time.h>
#include <EGSDK\GamePH\PlayerState.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\ClassHelpers.h>

namespace EGSDK::GamePH {
	static constexpr int STRING_SIZE_OFFSET = 3;
	static constexpr int FLOAT_SIZE_OFFSET = 3;
	static constexpr int BOOL_SIZE_OFFSET = 2;

	PlayerVar::PlayerVar(const std::string& name) : VarBase(name) {}
	PlayerVar::PlayerVar(const std::string& name, Engine::VarType type) : Engine::VarBase(name, type) {}
	Engine::VarValueType PlayerVar::GetValue() {
		std::lock_guard lock(mutex);
		switch (GetType()) {
			case Engine::VarType::String:
				return reinterpret_cast<const char*>(reinterpret_cast<uint64_t>(this->strValue.data) & 0x1FFFFFFFFFFFFFFF);
			case Engine::VarType::Float:
				return this->floatValue;
			case Engine::VarType::Bool:
				return this->boolValue;
			default:
				return {};
		}
	}
	Engine::VarValueType PlayerVar::GetDefaultValue() {
		std::lock_guard lock(mutex);
		switch (GetType()) {
			case Engine::VarType::String:
				return reinterpret_cast<const char*>(reinterpret_cast<uint64_t>(this->defaultStrValue.data) & 0x1FFFFFFFFFFFFFFF);
			case Engine::VarType::Float:
				return this->defaultFloatValue;
			case Engine::VarType::Bool:
				return this->defaultBoolValue;
			default:
				return {};
		}
	}
	void PlayerVar::SetValue(const Engine::VarValueType& value) {
		std::lock_guard lock(mutex);
		std::visit([&](auto&& val) {
			using T = std::decay_t<decltype(val)>;
			if constexpr (std::is_same_v<T, std::string>) {
				uint64_t firstByte = (reinterpret_cast<uint64_t>(this->strValue.data) >> 56) & 0xFF;
				uint64_t newValueAddr = reinterpret_cast<uint64_t>(val.c_str());
				if (firstByte != 0x0)
					newValueAddr |= (firstByte << 56);

				this->strValue = val.c_str();
				this->defaultStrValue = val.c_str();
			} else if constexpr (std::is_same_v<T, float>) {
				this->floatValue = val;
				this->defaultFloatValue = val;
			} else if constexpr (std::is_same_v<T, bool>) {
				this->boolValue = val;
				this->defaultBoolValue = val;
			}
		}, value);
	}

	StringPlayerVariable::StringPlayerVariable(const std::string& name) : PlayerVar(name) {
		SetType(Engine::VarType::String);
	}
	FloatPlayerVariable::FloatPlayerVariable(const std::string& name) : PlayerVar(name) {
		SetType(Engine::VarType::Float);
	}
	BoolPlayerVariable::BoolPlayerVariable(const std::string& name) : PlayerVar(name) {
		SetType(Engine::VarType::Bool);
	}

	PlayerVarMap PlayerVariables::customDefaultVars{};
	std::atomic<bool> PlayerVariables::gotPlayerVars = false;
	static bool sortedPlayerVars = false;

#pragma region Player Variables Processing
	template <typename T>
	static void updateDefaultVar(PlayerVarMap& defaultVars, const std::string& name, T value, T defaultValue) {
		static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, float> || std::is_same_v<T, bool>, "Invalid type: value must be string, float or bool");

		auto playerVar = defaultVars.Find(name);
		if (!playerVar) {
            if constexpr (std::is_same_v<T, std::string>) {
				auto stringPlayerVar = std::make_unique<StringPlayerVariable>(name);
				defaultVars.try_emplace(std::move(stringPlayerVar));
            } else if constexpr (std::is_same_v<T, float>) {
				auto floatPlayerVar = std::make_unique<FloatPlayerVariable>(name);
				floatPlayerVar->SetValue(value);
				defaultVars.try_emplace(std::move(floatPlayerVar));
            } else if constexpr (std::is_same_v<T, bool>) {
				auto boolPlayerVar = std::make_unique<BoolPlayerVariable>(name);
				boolPlayerVar->SetValue(value);
				defaultVars.try_emplace(std::move(boolPlayerVar));
            }
		} else {
			if constexpr (std::is_same_v<T, std::string>) {
				// TO IMPLEMENT
				return;
			} else if constexpr (std::is_same_v<T, float>) {
				auto floatPlayerVar = reinterpret_cast<FloatPlayerVariable*>(playerVar);
				floatPlayerVar->SetValue(value);
			} else if constexpr (std::is_same_v<T, bool>) {
				auto boolPlayerVar = reinterpret_cast<BoolPlayerVariable*>(playerVar);
				boolPlayerVar->SetValue(value);
			}
		}
	}
	static void processPlayerVar(uint64_t*(*playerVarsGetter)(), std::unique_ptr<PlayerVar>& playerVarPtr) {
		static int offset = 0;
		int offsetDif = 0;
		while (true) {
			std::string vTableName = Utils::RTTI::GetVTableName(playerVarsGetter() + offset);
			if (vTableName != "StringPlayerVariable" && vTableName != "FloatPlayerVariable" && vTableName != "BoolPlayerVariable") {
				if (offsetDif > 150)
					return;

				offset += 1;
				offsetDif += 1;
				continue;
			}

			std::string varName = playerVarPtr->GetName();
			Engine::VarType varType = playerVarPtr->GetType();

			switch (playerVarPtr->GetType()) {
			case Engine::VarType::String:
			{
				if (vTableName != "StringPlayerVariable")
					return;

				StringPlayerVariable* stringPlayerVar = reinterpret_cast<StringPlayerVariable*>(playerVarsGetter() + offset);
				playerVarPtr.reset(stringPlayerVar);
				playerVarPtr->SetName(varName);
				playerVarPtr->SetType(varType);
				// TO IMPLEMENT

				offset += STRING_SIZE_OFFSET;
				return;
			}
			case Engine::VarType::Float:
			{
				if (vTableName != "FloatPlayerVariable")
					return;

				FloatPlayerVariable* floatPlayerVar = reinterpret_cast<FloatPlayerVariable*>(playerVarsGetter() + offset);
				playerVarPtr.reset(floatPlayerVar);
				playerVarPtr->SetName(varName);
				playerVarPtr->SetType(varType);
				updateDefaultVar(PlayerVariables::customDefaultVars, varName, floatPlayerVar->floatValue.data, floatPlayerVar->defaultFloatValue.data);

				offset += FLOAT_SIZE_OFFSET;
				return;
			}
			case Engine::VarType::Bool:
			{
				if (vTableName != "BoolPlayerVariable")
					return;

				BoolPlayerVariable* boolPlayerVar = reinterpret_cast<BoolPlayerVariable*>(playerVarsGetter() + offset);
				playerVarPtr.reset(boolPlayerVar);
				playerVarPtr->SetName(varName);
				playerVarPtr->SetType(varType);
				updateDefaultVar(PlayerVariables::customDefaultVars, varName, boolPlayerVar->boolValue.data, boolPlayerVar->defaultBoolValue.data);

				offset += BOOL_SIZE_OFFSET;
				return;
			}
			default:
				offset += 1;
				return;
			}
		}
	}
	static void processPlayerVarSafe(std::unique_ptr<PlayerVar>& playerVarPtr, uint64_t*(*playerVarsGetter)()) {
		__try {
			processPlayerVar(playerVarsGetter, playerVarPtr);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			SPDLOG_ERROR("Failed to process player variable: {}", playerVarPtr->GetName());
		}
	}

	void PlayerVariables::GetPlayerVars() {
		if (gotPlayerVars)
			return;
		if (!sortedPlayerVars)
			return;
		if (!Get())
			return;

		customVars.reserve(vars.size());
		defaultVars.reserve(vars.size());
		customDefaultVars.reserve(vars.size());

		vars.ForEach(processPlayerVarSafe, reinterpret_cast<uint64_t*(*)()>(&Get));
		gotPlayerVars = true;
	}
#pragma endregion

#pragma region Player Variables Sorting
	struct VarTypeFieldMeta {
		Engine::VarType type;
		std::string_view className;
	};
	const std::vector<VarTypeFieldMeta> varTypeFields = {
		{ Engine::VarType::String, "constds::FieldsCollection<PlayerVariables>::TypedFieldMeta<StringPlayerVariable>" },
		{ Engine::VarType::Float, "constds::FieldsCollection<PlayerVariables>::TypedFieldMeta<FloatPlayerVariable>" },
		{ Engine::VarType::Bool, "constds::FieldsCollection<PlayerVariables>::TypedFieldMeta<BoolPlayerVariable>" }
	};

	static bool isRetInstruction(uint8_t* address) {
		//return address[0] == 0xC3 && address[1] == 0xCC;
		return address[0] == 0x00 && address[1] == 0x00 && address[2] == 0xC3 && address[3] == 0xCC;
	}
	static bool isLeaInstruction(uint8_t* address, uint8_t REX, uint8_t ModRM) {
		return address[0] == REX && address[1] == 0x8D && address[2] == ModRM;
	}
	static bool isCallInstruction(uint8_t* address) {
		return address[0] == 0xE8 && address[4] != 0xE8;
	}
	static bool isBelowFuncSizeLimit(uint8_t* address, uint64_t startOfFunc, size_t sizeLimit) {
		return (reinterpret_cast<uint64_t>(address) - startOfFunc) < sizeLimit;
	}

	// to prevent infinite loops, assuming function is no longer than 500000 uint8_ts LMAO Techland... why is your function even like 250000 uint8_ts to begin with? bad code...
	static const size_t MAX_FUNC_SIZE = 500000;
	static const size_t MAX_LOAD_VAR_FUNC_SIZE = 2000;

	static const char* getPlayerVarName(uint8_t*& funcAddress, uint64_t startOfFunc) {
		const char* playerVarName = nullptr;
		while (!playerVarName && !isRetInstruction(funcAddress) && isBelowFuncSizeLimit(funcAddress, startOfFunc, MAX_FUNC_SIZE)) {
			// lea r8, varNameString
			if (!isLeaInstruction(funcAddress, 0x4C, 0x05)) {
				funcAddress++;
				continue;
			}

			playerVarName = reinterpret_cast<const char*>(Utils::Memory::CalcTargetAddrOfRelativeInstr(reinterpret_cast<uint64_t>(funcAddress), 3));
			if (!playerVarName) {
				funcAddress++;
				continue;
			}

			// add the size of the instruction, so we skip this instruction because this instruction is the name
			funcAddress += 0x7;
		}

		return playerVarName;
	}
	static Engine::VarType getPlayerVarType(uint8_t*& funcAddress, uint64_t startOfFunc) {
		Engine::VarType varType = Engine::VarType::NONE;

		while (varType == Engine::VarType::NONE && !isRetInstruction(funcAddress) && isBelowFuncSizeLimit(funcAddress, startOfFunc, MAX_FUNC_SIZE)) {
			// call LoadPlayerXVariable
			if (!isCallInstruction(funcAddress)) {
				funcAddress++;
				continue;
			}

			uint64_t startOfLoadVarFunc = Utils::Memory::CalcTargetAddrOfRelativeInstr(reinterpret_cast<uint64_t>(funcAddress), 1);
			uint8_t* loadVarFuncAddress = reinterpret_cast<uint8_t*>(startOfLoadVarFunc);
			uint64_t metaVTAddrFromFunc = 0;

			while (!metaVTAddrFromFunc && !isRetInstruction(loadVarFuncAddress) && isBelowFuncSizeLimit(loadVarFuncAddress, startOfLoadVarFunc, MAX_LOAD_VAR_FUNC_SIZE)) {
				// lea rax, typedFieldMetaVT
				if (!isLeaInstruction(loadVarFuncAddress, 0x48, 0x05)) {
					loadVarFuncAddress++;
					continue;
				}

				metaVTAddrFromFunc = Utils::Memory::CalcTargetAddrOfRelativeInstr(reinterpret_cast<uint64_t>(loadVarFuncAddress), 3);
				std::string vTableName = Utils::RTTI::GetVTableNameFromVTPtr(reinterpret_cast<uint64_t*>(metaVTAddrFromFunc));
				auto varTypeIt = std::find_if(varTypeFields.begin(), varTypeFields.end(), [&vTableName](const auto& varType) {
					return varType.className == vTableName;
				});
				if (varTypeIt == varTypeFields.end()) {
					metaVTAddrFromFunc = 0;
					loadVarFuncAddress++;
					continue;
				}

				varType = varTypeIt->type;
				break;
			}

			// if it's still NONE after seeing the function doesnt reference any of the variables, break so the loop stops
			if (varType == Engine::VarType::NONE)
				break;
		}

		return varType;
	}

	bool PlayerVariables::SortPlayerVars() {
		static Utils::Time::Timer timeSpentSorting{ 20000 };
		uint64_t startOfFunc = 0;
		while (true) {
			if (timeSpentSorting.DidTimePass()) {
				SPDLOG_ERROR("Sorting player variables timed out because it couldn't get offset to LoadPlayerVars");
				return false;
			}

			if (!startOfFunc)
				startOfFunc = reinterpret_cast<uint64_t>(OffsetManager::Get_LoadPlayerVars());
			if (startOfFunc)
				break;

			Sleep(1000);
		}

		uint8_t* funcAddress = reinterpret_cast<uint8_t*>(startOfFunc);
		while (!isRetInstruction(funcAddress) && (reinterpret_cast<uint64_t>(funcAddress) - startOfFunc) < MAX_FUNC_SIZE) {
			const char* playerVarName = getPlayerVarName(funcAddress, startOfFunc);
			if (!playerVarName)
				continue;

			Engine::VarType playerVarType = getPlayerVarType(funcAddress, startOfFunc);
			switch (playerVarType) {
				case Engine::VarType::String:
					vars.try_emplace(std::make_unique<StringPlayerVariable>(playerVarName));
					break;
				case Engine::VarType::Float:
					vars.try_emplace(std::make_unique<FloatPlayerVariable>(playerVarName));
					break;
				case Engine::VarType::Bool:
					vars.try_emplace(std::make_unique<BoolPlayerVariable>(playerVarName));
					break;
				default:
					//vars.try_emplace(std::make_unique<PlayerVar>(playerVarName));
					break;
			}
		}

		sortedPlayerVars = true;
		return true;
	}
#pragma endregion

	static PlayerVariables* GetOffset_PlayerVariables() {
		PlayerState* playerState = PlayerState::Get();
		return playerState ? playerState->playerVariables : nullptr;
	}
	PlayerVariables* PlayerVariables::Get() {
		return ClassHelpers::SafeGetter<PlayerVariables>(GetOffset_PlayerVariables, false, false);
	}
}