#include <algorithm>
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
    std::unordered_map<std::string, std::any> VarManagerBase<VarMapT, VarT>::prevVarValueMap{};
    template <typename VarMapT, typename VarT>
    std::unordered_map<std::string, bool> VarManagerBase<VarMapT, VarT>::prevBoolValueMap{};
    template <typename VarMapT, typename VarT>
    std::unordered_map<std::string, uint64_t> VarManagerBase<VarMapT, VarT>::varOwnerMap{};

    template <typename VarMapT, typename VarT>
    std::mutex VarManagerBase<VarMapT, VarT>::writeMutex{};
    template <typename VarMapT, typename VarT>
    std::shared_mutex VarManagerBase<VarMapT, VarT>::readMutex{};

    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::GetVarRefFromPtr(VarT* var) {
        return var ? std::optional<VarRef<VarMapT, VarT>>(VarRef<VarMapT, VarT>(var)) : std::nullopt;
    }
    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::GetVarRef(const char* name) {
        return _GetVarRef(name, vars);
    }
    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::GetCustomVarRef(const char* name) {
        return _GetVarRef(name, customVars);
    }
    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::GetDefaultVarRef(const char* name) {
        return _GetVarRef(name, defaultVars);
    }
    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::GetCustomDefaultVarRef(const char* name) {
        return _GetVarRef(name, defaultCustomVars);
    }

    template <typename VarMapT, typename VarT>
    std::optional<VarRef<VarMapT, VarT>> VarManagerBase<VarMapT, VarT>::_GetVarRef(const char* name, VarMapT& map) {
        VarRef<VarMapT, VarT> varRef(name, map);
        return varRef.GetPtr() ? std::optional<VarRef<VarMapT, VarT>>(varRef) : std::nullopt;
    }

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
        std::shared_lock lock(readMutex);
        return (prevBoolValueMap.find(name) != prevBoolValueMap.end()) && prevBoolValueMap[name];
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::_IsManagedByBool(VarRef<VarMapT, VarT>* var) {
        return _IsManagedByBool(var->GetName());
    }
    template <typename VarMapT, typename VarT>
    bool VarManagerBase<VarMapT, VarT>::_HasCustomValue(VarRef<VarMapT, VarT>* var) {
        return !customVars.none_of(var->GetName());
    }

    template class VarManagerBase<CVarMap, CVar>;
    template class VarManagerBase<GamePH::PlayerVarMap, GamePH::PlayerVar>;
}