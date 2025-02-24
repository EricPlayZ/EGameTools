#pragma once
#include <functional>
#include <vector>
#include <mutex>
#include <shared_mutex>

namespace EGT::ImGui_impl {
    class DeferredActions {
    public:
        static void Add(const std::function<void()>& action);
        static void Clear();

        static bool HasPendingActions();
        static void Process();
    private:
        static std::vector<std::function<void()>> actions;
        static std::mutex writeMutex;
        static std::shared_mutex readMutex;
    };
}