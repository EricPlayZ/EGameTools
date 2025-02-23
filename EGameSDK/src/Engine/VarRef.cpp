#include <EGSDK\Utils\Values.h>
#include <EGSDK\Engine\VarRef.h>
#include <EGSDK\Engine\VarBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>

namespace EGSDK::Engine {
    template <typename VarMapT, typename VarT>
    VarRef<VarMapT, VarT>::VarRef(VarT* var) : name(var ? var->GetName() : nullptr), ptr(var) {}
    template <typename VarMapT, typename VarT>
    VarRef<VarMapT, VarT>::VarRef(const char* name, VarMapT& map) : name(name), ptr(map.Find(name)) {}

    template <typename VarMapT, typename VarT>
    const char* VarRef<VarMapT, VarT>::GetName() const {
        return name;
    }
    template <typename VarMapT, typename VarT>
    VarType VarRef<VarMapT, VarT>::GetType() const {
        return ptr ? ptr->GetType() : VarType::NONE;
    }
    template <typename VarMapT, typename VarT>
    VarT* VarRef<VarMapT, VarT>::GetPtr() const {
        return ptr;
    }

    template <typename VarMapT, typename VarT>
    bool VarRef<VarMapT, VarT>::IsManagedByBool() {
        return VarMgrBase::_IsManagedByBool(this);
    }
    template <typename VarMapT, typename VarT>
    bool VarRef<VarMapT, VarT>::HasCustomValue() {
        return VarMgrBase::_HasCustomValue(this);
    }

    template class VarRef<CVarMap, CVar>;
    template class VarRef<GamePH::PlayerVarMap, GamePH::PlayerVar>;
}