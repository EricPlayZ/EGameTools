#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <string>
#include <EGSDK\Exports.h>

namespace EGSDK::Engine {
    template <typename VarT>
    class EGameSDK_API VarMapBase {
    public:
        VarMapBase() = default;
        VarMapBase(const VarMapBase&) = delete;
        VarMapBase& operator=(const VarMapBase&) = delete;
        VarMapBase(VarMapBase&&) noexcept = default;
        VarMapBase& operator=(VarMapBase&&) noexcept = default;

        virtual ~VarMapBase() = default;

        virtual std::unique_ptr<VarT>& try_emplace(std::unique_ptr<VarT> var);
        bool empty() const;
        bool none_of(const std::string& name) const;
        void reserve(size_t count);
        size_t size();

        VarT* Find(const std::string& name) const;

        virtual void Erase(const std::string& name);

        template <typename Callable, typename... Args>
        void ForEach(Callable&& func, Args&&... args) {
            std::lock_guard lock(mutex);
            for (const auto& name : varsOrdered)
                func(vars.at(name), std::forward<Args>(args)...);
        }
    protected:
        std::unordered_map<std::string, std::unique_ptr<VarT>> vars{};
        std::vector<std::string> varsOrdered{};
        mutable std::recursive_mutex mutex{};
    };
}