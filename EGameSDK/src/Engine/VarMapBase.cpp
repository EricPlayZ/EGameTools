#include <algorithm>
#include <EGSDK\Engine\VarMapBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>

namespace EGSDK::Engine {
    template <typename VarT>
    VarMapBase<VarT>::VarMapBase() : vars(), varsOrdered(), writeMutex(), readMutex() {}

    template <typename VarT>
    std::unique_ptr<VarT>& VarMapBase<VarT>::AddVar(std::unique_ptr<VarT> var) {
        std::lock_guard lock(writeMutex);
        const char* name = var->GetName();
        auto [it, inserted] = vars.try_emplace(name, std::move(var));
        if (inserted)
            varsOrdered.push_back(name);
        else
            var.release();
        return it->second;
    }
    template <typename VarT>
    VarT* VarMapBase<VarT>::Find(std::string_view name) const {
        std::shared_lock lock(readMutex);
        auto it = vars.find(name);
        return (it != vars.end()) ? it->second.get() : nullptr;
    }
    template <typename VarT>
    void VarMapBase<VarT>::Erase(std::string_view name) {
        std::lock_guard lock(writeMutex);
        auto it = vars.find(name);
        if (it == vars.end())
            return;
        auto orderIt = std::find(varsOrdered.begin(), varsOrdered.end(), name);
        if (orderIt != varsOrdered.end())
            varsOrdered.erase(orderIt);
        vars.erase(it);
    }

    template <typename VarT>
    bool VarMapBase<VarT>::empty() const {
        std::shared_lock lock(readMutex);
        return vars.empty();
    }
    template <typename VarT>
    bool VarMapBase<VarT>::none_of(std::string_view name) const {
        std::shared_lock lock(readMutex);
        return vars.find(name) == vars.end();
    }
    template <typename VarT>
    size_t VarMapBase<VarT>::size() {
        std::shared_lock lock(readMutex);
        return vars.size();
    }
    template <typename VarT>
    void VarMapBase<VarT>::reserve(size_t count) {
        std::lock_guard lock(writeMutex);
        vars.reserve(count);
        varsOrdered.reserve(count);
    }

    template class VarMapBase<CVar>;
    template class VarMapBase<GamePH::PlayerVar>;
}