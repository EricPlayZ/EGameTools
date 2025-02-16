#pragma once
#include <string>
#include <any>
#include <mutex>
#include <unordered_map>
#include <intrin.h>
#include <variant>
#include <optional>
#include <EGSDK\Exports.h>
#include <EGSDK\Vec3.h>
#include <EGSDK\Vec4.h>
#include <EGSDK\Utils\Values.h>
#include <EGSDK\Engine\VarBase.h>

#pragma intrinsic(_ReturnAddress)

namespace EGSDK::Engine {
#define StaticAssertValueType static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4> || std::is_same_v<T, bool>, "Invalid type: value must be string, float, int, Vec3, Vec4 or bool")

    template <typename VarMapType, typename VarType>
    class EGameSDK_API VarManagerBase {
    public:
        static VarMapType vars;
        static VarMapType customVars;
        static VarMapType defaultVars;

        template <typename T>
        static std::optional<T> GetVarValue(const std::string& name) {
            StaticAssertValueType;
            auto var = vars.Find(name);
            if (!var)
                return std::nullopt;
            auto value = var->GetValue();
            auto variantValue = std::get_if<T>(&value);
            return variantValue ? std::optional<T>(*variantValue) : std::nullopt;
        }
        template <typename T>
        static std::optional<T> GetVarValue(VarType* var) {
            StaticAssertValueType;
            if (!var)
                return std::nullopt;
            auto value = var->GetValue();
            auto variantValue = std::get_if<T>(&value);
            return variantValue ? std::optional<T>(*variantValue) : std::nullopt;
        }
        template <typename T>
        static std::optional<T> GetVarValueFromMap(const std::string& name, const VarMapType& map) {
            StaticAssertValueType;
            auto var = map.Find(name);
            return GetVarValue<T>(var);
        }

        static VarType* GetVar(const std::string& name);

        template <typename T>
        static void ChangeVar(const std::string& name, T value) {
            StaticAssertValueType;
            auto var = vars.Find(name);
            if (!var)
                return;

            ChangeVar<T>(var, value);
        }
        template <typename T>
        static void ChangeVar(VarType* var, T value) {
            StaticAssertValueType;

            if (!var)
                return;

            if constexpr (std::is_same_v<T, std::string>) {
                switch (var->GetType()) {
                    case Engine::VarType::Float:
                        ChangeVar<float>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    case Engine::VarType::Int:
                        ChangeVar<int>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    case Engine::VarType::Bool:
                        ChangeVar<bool>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    default:
                        break;
                }
            }
            var->SetValue(value);
        }
        template <typename T>
        static void ChangeVarFromMap(const std::string& name, T value, const VarMapType& map) {
            StaticAssertValueType;
            auto var = map.Find(name);
            ChangeVar<T>(var, value);
        }
        template <typename T>
        static void ChangeVarFromList(const std::string& name, T value) {
            StaticAssertValueType;

            auto var = vars.Find(name);
            if (!var)
                return;

            ChangeVarFromList<T>(var, value);
        }
        template <typename T>
        static void ChangeVarFromList(VarType* var, T value) {
            StaticAssertValueType;

            if (!var)
                return;

            auto customVar = customVars.Find(var->GetName());
            auto defVar = defaultVars.Find(var->GetName());

            if constexpr (std::is_same_v<T, std::string>) {
                switch (var->GetType()) {
                    case Engine::VarType::Float:
                        ChangeVarFromList<float>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    case Engine::VarType::Int:
                        ChangeVarFromList<int>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    case Engine::VarType::Bool:
                        ChangeVarFromList<bool>(var, std::stof(Utils::Values::to_string(value)));
                        return;
                    default:
                        break;
                }
            }
            if (!customVar)
                customVar = customVars.try_emplace(std::make_unique<VarType>(var->GetName(), var->GetType())).get();
            if (!defVar) {
                defVar = defaultVars.try_emplace(std::make_unique<VarType>(var->GetName(), var->GetType())).get();
                if (auto varValue = GetVarValue<T>(var); defVar && varValue)
                    defVar->SetValue(*varValue);
            }

            if (customVar)
                customVar->SetValue(value);
            var->SetValue(value);
        }

        template <typename T>
        static void ManageVarByBool(const std::string& name, T valueIfTrue, T valueIfFalse, bool boolVal, bool usePreviousVal = true) {
            uint64_t caller = reinterpret_cast<uint64_t>(_ReturnAddress());

            std::lock_guard lock(mutex);

            auto ownerIt = varOwnerMap.find(name);
            if (ownerIt != varOwnerMap.end() && ownerIt->second != caller)
                return;
            if (!boolVal && prevBoolValueMap.find(name) == prevBoolValueMap.end())
                return;

            bool& prevBoolValue = prevBoolValueMap[name];
            auto& prevValueAny = prevVarValueMap[name];

            if (boolVal) {
                if (!prevBoolValue) {
                    auto varValue = GetVarValue<T>(name);
                    prevVarValueMap[name] = varValue ? *varValue : T{};
                }

                ChangeVarFromList(name, valueIfTrue);
                prevBoolValue = true;
                varOwnerMap[name] = caller;
            } else if (prevBoolValue) {
                ChangeVarFromList(name, usePreviousVal ? std::any_cast<T>(prevValueAny) : valueIfFalse);
                defaultVars.Erase(name);
                customVars.Erase(name);
                prevVarValueMap.erase(name);
                prevBoolValueMap.erase(name);
                varOwnerMap.erase(name);
            }
        }
        static bool IsVarManagedByBool(const std::string& name);
        static bool DoesVarHaveCustomValue(const std::string& name);
        static bool AreAnyVarsPresent();
        static bool AreAnyCustomVarsPresent();
        static bool AreAllCustomVarsManagedByBool();

        template <typename T>
        static void RestoreVariableToDefault(VarType* var, bool eraseFromMaps = true) {
            if (IsVarManagedByBool(var->GetName()))
                return;

            auto defValue = GetVarValueFromMap<T>(var->GetName(), defaultVars);
            if (!defValue)
                return;

            ChangeVar(var->GetName(), *defValue);

            if (eraseFromMaps) {
                defaultVars.Erase(var->GetName());
                customVars.Erase(var->GetName());
            }
        }
        template <typename T>
        static void RestoreVariableToDefaultFromMap(VarType* var, VarMapType& map, bool eraseFromMaps = true) {
            if (IsVarManagedByBool(var->GetName()))
                return;

            auto defValue = GetVarValueFromMap<T>(var->GetName(), map);
            if (!defValue)
                return;

            ChangeVar(var->GetName(), *defValue);

            if (eraseFromMaps) {
                map.Erase(var->GetName());
                customVars.Erase(var->GetName());
            }
        }
    private:
        static std::unordered_map<std::string, std::any> prevVarValueMap;
        static std::unordered_map<std::string, bool> prevBoolValueMap;
        static std::unordered_map<std::string, uint64_t> varOwnerMap;
        static std::recursive_mutex mutex;
    };

#undef StaticAssertValueType
}