#pragma once
#include <optional>
#include <EGSDK\Engine\VarBase.h>

namespace EGSDK::Engine {
    template <typename VarMapT, typename VarT>
    class EGameSDK_API VarManagerBase;

    template <typename VarMapT, typename VarT>
    class EGameSDK_API VarRef {
    public:
        using VarMgrBase = VarManagerBase<VarMapT, VarT>;

        VarRef(VarT* var);
        VarRef(const char* name, VarMapT& map);

        const char* GetName() const;
        VarType GetType() const;
        VarT* GetPtr() const;

        template <AllowedVarTypes T>
        std::optional<T> GetValue() const {
            if (!ptr)
                return std::nullopt;

            auto value = ptr->GetValue();
            if (auto* typedValue = std::get_if<T>(&value))
                return *typedValue;
            return std::nullopt;
        }
        template <AllowedVarTypes T>
        void SetValue(T value) {
            if (!ptr)
                return;

            if constexpr (std::is_same_v<T, std::string>) {
                std::string valueStr = Utils::Values::to_string(value);
                switch (ptr->GetType()) {
                    case VarType::Float:
                        SetValue<float>(std::stof(valueStr));
                        return;
                    case VarType::Int:
                        SetValue<int>(std::stof(valueStr));
                        return;
                    case VarType::Bool:
                        SetValue<bool>(!_strcmpi(valueStr.c_str(), "true"));
                        return;
                    default:
                        break;
                }
            }
            ptr->SetValue(std::move(value));
        }
        template <AllowedVarTypes T>
        void SetValueFromList(T value) {
            VarMgrBase::template _SetValueFromList<T>(this, value);
        }

        bool IsManagedByBool();
        bool HasCustomValue();

        template <AllowedVarTypes T>
        void ManageByBool(T valueIfTrue, T valueIfFalse, bool boolVal, bool usePreviousVal = true) {
            VarMgrBase::template _ManageByBool<T>(this, valueIfTrue, valueIfFalse, boolVal, usePreviousVal);
        }
        template <AllowedVarTypes T>
        void SaveVariableAsDefault() {
            VarMgrBase::template _SaveVariableAsDefault<T>(this);
        }
        template <AllowedVarTypes T>
        void RestoreVarToDefault(bool restoreToSavedVars = false) {
            VarMgrBase::template _RestoreVarToDefault<T>(this, restoreToSavedVars);
        }
    protected:
        const char* name;
        VarT* ptr = nullptr;
    };
}