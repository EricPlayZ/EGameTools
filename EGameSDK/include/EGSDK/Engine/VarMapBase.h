#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <string_view>
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

        virtual std::unique_ptr<VarT>& AddVar(std::unique_ptr<VarT> var);
        VarT* Find(std::string_view name) const;
        virtual void Erase(std::string_view name);

        bool empty() const;
        bool none_of(std::string_view name) const;
        size_t size();
        void reserve(size_t count);

        template <typename Callable, typename... Args>
        void ForEach(Callable&& func, Args&&... args) {
            std::shared_lock lock(readMutex);
            for (const auto& name : varsOrdered)
                func(vars.at(name), std::forward<Args>(args)...);
        }
    private:
        struct CaseInsensitiveHash {
            size_t operator()(std::string_view s) const {
                size_t h = 0;
                for (char c : s)
                    h = h * 101 + static_cast<size_t>(std::tolower(static_cast<unsigned char>(c)));
                return h;
            }
        };
        struct CaseInsensitiveEqual {
            bool operator()(std::string_view lhs, std::string_view rhs) const {
                return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                    [](unsigned char a, unsigned char b) {
                    return std::tolower(a) == std::tolower(b);
                });
            }
        };
    protected:
        std::unordered_map<std::string_view, std::unique_ptr<VarT>, CaseInsensitiveHash, CaseInsensitiveEqual> vars;
        std::vector<std::string_view> varsOrdered;
        mutable std::mutex writeMutex;
        mutable std::shared_mutex readMutex;
    };
}