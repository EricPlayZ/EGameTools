#include <EGSDK\Engine\VarManagerBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>
#include <EGSDK\Vec4.h>

namespace EGSDK::Engine {
    template <typename VarMapType, typename VarType>
    VarMapType VarManagerBase<VarMapType, VarType>::vars{};
    template <typename VarMapType, typename VarType>
    VarMapType VarManagerBase<VarMapType, VarType>::customVars{};
    template <typename VarMapType, typename VarType>
    VarMapType VarManagerBase<VarMapType, VarType>::defaultVars{};

    template <typename VarMapType, typename VarType>
    std::recursive_mutex VarManagerBase<VarMapType, VarType>::mutex{};

#ifdef EGameSDK_EXPORTS
    template <typename VarMapType, typename VarType>
    std::unordered_map<std::string, std::any> VarManagerBase<VarMapType, VarType>::prevVarValueMap{};
    template <typename VarMapType, typename VarType>
    std::unordered_map<std::string, bool> VarManagerBase<VarMapType, VarType>::prevBoolValueMap{};
    template <typename VarMapType, typename VarType>
    std::unordered_map<std::string, uint64_t> VarManagerBase<VarMapType, VarType>::varOwnerMap{};
#endif

    template <typename VarMapType, typename VarType>
    VarType* VarManagerBase<VarMapType, VarType>::GetVar(const std::string& name) {
        return vars.Find(name);
    }

    template <typename VarMapType, typename VarType>
    bool VarManagerBase<VarMapType, VarType>::IsVarManagedByBool(const std::string& name) {
        std::lock_guard lock(mutex);
        return prevBoolValueMap.find(name) != prevBoolValueMap.end() && prevBoolValueMap[name];
    }
    template <typename VarMapType, typename VarType>
    bool VarManagerBase<VarMapType, VarType>::DoesVarHaveCustomValue(const std::string& name) {
        return !customVars.none_of(name);
    }
    template <typename VarMapType, typename VarType>
    bool VarManagerBase<VarMapType, VarType>::AreAnyVarsPresent() {
        return !vars.empty();
    }
    template <typename VarMapType, typename VarType>
    bool VarManagerBase<VarMapType, VarType>::AreAnyCustomVarsPresent() {
        return !customVars.empty();
    }
    template <typename VarMapType, typename VarType>
    bool VarManagerBase<VarMapType, VarType>::AreAllCustomVarsManagedByBool() {
        bool allManagedByBool = true;

        customVars.ForEach([&allManagedByBool](const std::unique_ptr<VarType>& varPtr) {
            if (!IsVarManagedByBool(varPtr->GetName())) {
                allManagedByBool = false;
                return;
            }
        });

        return allManagedByBool;
    }

    template EGameSDK_API class VarManagerBase<CVarMap, CVar>;
    template EGameSDK_API class VarManagerBase<GamePH::PlayerVarMap, GamePH::PlayerVar>;
}