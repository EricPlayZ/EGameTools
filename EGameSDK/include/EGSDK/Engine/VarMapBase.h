#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <string>
#include <type_traits>
#include <EGSDK\Exports.h>
#include <EGSDK\Engine\VarBase.h>

namespace EGSDK::Engine {
    template <typename VarT>
    class EGameSDK_API VarMapBase {
        static_assert(std::is_base_of_v<VarBase, VarT>, "VarT must inherit from VarBase");
    public:
        VarMapBase();
        VarMapBase(const VarMapBase&) = delete;
        VarMapBase& operator=(const VarMapBase&) = delete;
        VarMapBase(VarMapBase&&) noexcept = default;
        VarMapBase& operator=(VarMapBase&&) noexcept = default;
        virtual ~VarMapBase() = default;

        virtual std::unique_ptr<VarT>& try_emplace(std::unique_ptr<VarT> var);
        VarT* Find(const std::string& name) const;
        virtual void Erase(const std::string& name);

        bool empty() const;
        bool none_of(const std::string& name) const;
        size_t size();
        void reserve(size_t count);

        template <typename Callable, typename... Args>
        void ForEach(Callable&& func, Args&&... args) {
            std::shared_lock lock(readMutex);
            for (const auto& name : varsOrdered)
                func(vars.at(name), std::forward<Args>(args)...);
        }
    protected:
        std::unordered_map<std::string, std::unique_ptr<VarT>> vars;
        std::vector<std::string> varsOrdered;
        mutable std::mutex writeMutex;
        mutable std::shared_mutex readMutex;
    };
}