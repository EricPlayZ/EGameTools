#pragma once
#include <string>
#include <any>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <intrin.h>
#include <variant>
#include <optional>
#include <EGSDK\Exports.h>
#include <EGSDK\Vec3.h>
#include <EGSDK\Vec4.h>
#include <EGSDK\Utils\Values.h>
#include <EGSDK\Engine\VarBase.h>
#include <EGSDK\Engine\VarRef.h>
#include <EGSDK\Engine\VarMapBase.h>

#pragma intrinsic(_ReturnAddress)

namespace EGSDK::Engine {
    template <typename VarMapT, typename VarT>
    class EGameSDK_API VarManagerBase {
        template <typename, typename>
        friend class VarRef;
    public:
        static VarMapT vars;
        static VarMapT customVars;
        static VarMapT defaultVars;
        static VarMapT defaultCustomVars;

        static std::optional<VarRef<VarMapT, VarT>> GetVarRefFromPtr(VarT* var);
        static std::optional<VarRef<VarMapT, VarT>> GetVarRef(const char* name);
        static std::optional<VarRef<VarMapT, VarT>> GetCustomVarRef(const char* name);
        static std::optional<VarRef<VarMapT, VarT>> GetDefaultVarRef(const char* name);
        static std::optional<VarRef<VarMapT, VarT>> GetCustomDefaultVarRef(const char* name);

        static bool AreAnyVarsPresent();
        static bool AreAnyCustomVarsPresent();
        static bool AreAllCustomVarsManagedByBool();

        template <AllowedVarTypes T>
        static void ManageVarByBool(const char* name, T valueIfTrue, T valueIfFalse, bool boolVal, bool usePreviousVal = true) {
            auto playerVar = GetVarRef(name);
            if (playerVar)
                _ManageByBool(_ReturnAddress(), &*playerVar, valueIfTrue, valueIfFalse, boolVal, usePreviousVal);
        }
    private:
        static std::unordered_map<std::string, std::any> prevVarValueMap;
        static std::unordered_map<std::string, bool> prevBoolValueMap;
        static std::unordered_map<std::string, uint64_t> varOwnerMap;
        static std::mutex writeMutex;
        static std::shared_mutex readMutex;

        static std::optional<VarRef<VarMapT, VarT>> _GetVarRef(const char* name, VarMapT& map);

        static bool _IsManagedByBool(const char* name);
        static bool _IsManagedByBool(VarRef<VarMapT, VarT>* var);
        static bool _HasCustomValue(VarRef<VarMapT, VarT>* var);

        template <AllowedVarTypes T>
        static void _SetValueFromList(VarRef<VarMapT, VarT>* var, T value) {
            if (!var)
                return;

            auto name = var->GetName();

            auto customVar = GetCustomVarRef(name);
            auto defVar = GetDefaultVarRef(name);

            if constexpr (std::is_same_v<T, std::string>) {
                std::string valueStr = Utils::Values::to_string(value);
                switch (var->GetType()) {
                    case VarType::Float:
                        _SetValueFromList<float>(var, std::stof(valueStr));
                        return;
                    case VarType::Int:
                        _SetValueFromList<int>(var, std::stof(valueStr));
                        return;
                    case VarType::Bool:
                        _SetValueFromList<bool>(var, !_strcmpi(valueStr.c_str(), "true"));
                        return;
                    default:
                        break;
                }
            }
            if (!customVar)
                customVar = customVars.AddVar(std::make_unique<VarT>(name, var->GetType())).get();
            if (!defVar) {
                defVar = defaultVars.AddVar(std::make_unique<VarT>(name, var->GetType())).get();
                if (auto varValue = var->GetValue<T>(); defVar && varValue)
                    defVar->SetValue<T>(*varValue);
            }

            if (customVar)
                customVar->SetValue(value);
            var->SetValue(value);
        }
        template <AllowedVarTypes T>
        static void _ManageByBool(void* returnAddr, VarRef<VarMapT, VarT>* var, T valueIfTrue, T valueIfFalse, bool boolVal, bool usePreviousVal = true) {
            if (!var)
                return;

            uint64_t caller = reinterpret_cast<uint64_t>(returnAddr);
            std::shared_lock lock(readMutex);

            const char* name = var->GetName();

            auto ownerIt = varOwnerMap.find(name);
            if (ownerIt != varOwnerMap.end() && ownerIt->second != caller)
                return;
            if (!boolVal && prevBoolValueMap.find(name) == prevBoolValueMap.end())
                return;

            bool& prevBoolValue = prevBoolValueMap[name];
            auto& prevValueAny = prevVarValueMap[name];

            if (boolVal) {
                if (!prevBoolValue) {
                    auto varValue = var->GetValue<T>();
                    prevVarValueMap[name] = varValue ? *varValue : T{};
                }

                _SetValueFromList<T>(var, valueIfTrue);
                prevBoolValue = true;
                varOwnerMap[name] = caller;
            } else if (prevBoolValue) {
                _SetValueFromList<T>(var, usePreviousVal ? std::any_cast<T>(prevValueAny) : valueIfFalse);
                defaultVars.Erase(name);
                customVars.Erase(name);
                prevVarValueMap.erase(name);
                prevBoolValueMap.erase(name);
                varOwnerMap.erase(name);
            }
        }
        template <AllowedVarTypes T>
        static void _SaveVarAsDefault(VarRef<VarMapT, VarT>* var) {
            if (!var)
                return;

            auto customDefaultVar = GetCustomDefaultVarRef(var->GetName());

            auto varValue = var->GetValue<T>();
            if (!varValue)
                return;
            customDefaultVar->SetValue(*varValue);
        }
        template <AllowedVarTypes T>
        static void _RestoreVarToDefault(VarRef<VarMapT, VarT>* var, bool restoreToSavedVars = false) {
            if (!var)
                return;
            if (_IsManagedByBool(var))
                return;

            const char* name = var->GetName();

            auto defVar = !restoreToSavedVars ? GetDefaultVarRef(name) : GetCustomDefaultVarRef(name);
            if (!defVar)
                return;
            auto defValue = defVar->GetValue<T>();
            if (!defValue)
                return;

            var->SetValue<T>(*defValue);

            if (!restoreToSavedVars) {
                defaultVars.Erase(name);
                customVars.Erase(name);
            }
        }
    };
}