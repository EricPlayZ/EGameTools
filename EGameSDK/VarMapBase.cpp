#include <EGSDK\Engine\VarMapBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>

namespace EGSDK::Engine {
    template <typename VarType>
    std::unique_ptr<VarType>& VarMapBase<VarType>::try_emplace(std::unique_ptr<VarType> var) {
        std::lock_guard lock(mutex);
        const std::string& name = var->GetName();
        auto [it, inserted] = vars.try_emplace(name, std::move(var));
        if (inserted)
            varsOrdered.emplace_back(name);
        return it->second;
    }
    template <typename VarType>
    bool VarMapBase<VarType>::empty() const {
        std::lock_guard lock(mutex);
        return vars.empty();
    }
    template <typename VarType>
    bool VarMapBase<VarType>::none_of(const std::string& name) const {
        std::lock_guard lock(mutex);
        return vars.find(name) == vars.end();
    }
    template <typename VarType>
    void VarMapBase<VarType>::reserve(size_t count) {
        std::lock_guard lock(mutex);
        vars.reserve(count);
    }
    template <typename VarType>
    size_t VarMapBase<VarType>::size() {
        std::lock_guard lock(mutex);
        return vars.size();
    }

    template <typename VarType>
    VarType* VarMapBase<VarType>::Find(const std::string& name) const {
        std::lock_guard lock(mutex);
        auto it = vars.find(name);
        return (it != vars.end()) ? it->second.get() : nullptr;
    }

    template <typename VarType>
    void VarMapBase<VarType>::Erase(const std::string& name) {
        std::lock_guard lock(mutex);
        auto it = vars.find(name);
        if (it == vars.end())
            return;

        auto orderIt = std::find(varsOrdered.begin(), varsOrdered.end(), name);
        if (orderIt != varsOrdered.end())
            varsOrdered.erase(orderIt);

        vars.erase(it);
    }

    template EGameSDK_API class VarMapBase<CVar>;
    template EGameSDK_API class VarMapBase<GamePH::PlayerVar>;
}