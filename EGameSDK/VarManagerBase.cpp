#include <EGSDK\Engine\VarManagerBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>

namespace EGSDK::Engine {
    template <typename VarMapT, typename VarT>
    VarMapT VarManagerBase<VarMapT, VarT>::vars{};
    template <typename VarMapT, typename VarT>
    VarMapT VarManagerBase<VarMapT, VarT>::customVars{};
    template <typename VarMapT, typename VarT>
    VarMapT VarManagerBase<VarMapT, VarT>::defaultVars{};
    template <typename VarMapT, typename VarT>
    VarMapT VarManagerBase<VarMapT, VarT>::defaultCustomVars{};

    template <typename VarMapT, typename VarT>
    std::recursive_mutex VarManagerBase<VarMapT, VarT>::mutex{};

    template <typename VarMapT, typename VarT>
    std::unordered_map<std::string, std::any> VarManagerBase<VarMapT, VarT>::prevVarValueMap{};
    template <typename VarMapT, typename VarT>
    std::unordered_map<std::string, bool> VarManagerBase<VarMapT, VarT>::prevBoolValueMap{};
    template <typename VarMapT, typename VarT>
    std::unordered_map<std::string, uint64_t> VarManagerBase<VarMapT, VarT>::varOwnerMap{};

    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::AreAnyVarsPresent() {
        return !vars.empty();
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::AreAnyCustomVarsPresent() {
        return !customVars.empty();
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::AreAllCustomVarsManagedByBool() {
        bool allManagedByBool = true;

        customVars.ForEach([&allManagedByBool](const std::unique_ptr<VarT>& varPtr) {
            if (!_IsManagedByBool(varPtr->GetName())) {
                allManagedByBool = false;
                return;
            }
        });

        return allManagedByBool;
    }

    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::_IsManagedByBool(const char* name) {
        std::lock_guard lock(mutex);
        return prevBoolValueMap.find(name) != prevBoolValueMap.end() && prevBoolValueMap[name];
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::_IsManagedByBool(VarRef<VarMapT, VarT>* var) {
        return _IsManagedByBool(var->GetName());
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::_HasCustomValue(VarRef<VarMapT, VarT>* var) {
        return !customVars.none_of(var->GetName());
    }

    template EGameSDK_API class VarManagerBase<CVarMap, CVar>;
    template EGameSDK_API class VarManagerBase<GamePH::PlayerVarMap, GamePH::PlayerVar>;
}