#include <algorithm>
#include <EGSDK\Engine\VarMapBase.h>
#include <EGSDK\Engine\CVars.h>
#include <EGSDK\GamePH\PlayerVariables.h>

namespace EGSDK::Engine {
    template <typename VarT>
    VarMapBase<VarT>::VarMapBase() : vars(), varsOrdered(), writingMutex(), readingMutex() {}

    template <typename VarT>
    std::unique_ptr<VarT>& VarMapBase<VarT>::try_emplace(std::unique_ptr<VarT> var) {
        std::lock_guard lock(writingMutex);
        const std::string& name = var->GetName();
        auto [it, inserted] = vars.try_emplace(name, std::move(var));
        if (inserted)
            varsOrdered.push_back(name);
        return it->second;
    }

    template <typename VarT>
    VarT* VarMapBase<VarT>::Find(const std::string& name) const {
        std::shared_lock lock(readingMutex);
        auto it = vars.find(name);
        return (it != vars.end()) ? it->second.get() : nullptr;
    }

    template <typename VarT>
    void VarMapBase<VarT>::Erase(const std::string& name) {
        std::lock_guard lock(writingMutex);
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
        std::shared_lock lock(readingMutex);
        return vars.empty();
    }

    template <typename VarT>
    bool VarMapBase<VarT>::none_of(const std::string& name) const {
        std::shared_lock lock(readingMutex);
        return vars.find(name) == vars.end();
    }

    template <typename VarT>
    size_t VarMapBase<VarT>::size() {
        std::shared_lock lock(readingMutex);
        return vars.size();
    }

    template <typename VarT>
    void VarMapBase<VarT>::reserve(size_t count) {
        std::lock_guard lock(writingMutex);
        vars.reserve(count);
        varsOrdered.reserve(count);
    }

    template class VarMapBase<CVar>;
    template class VarMapBase<GamePH::PlayerVar>;
}