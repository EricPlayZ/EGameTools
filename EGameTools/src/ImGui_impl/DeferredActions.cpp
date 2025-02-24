#include <EGT\ImGui_impl\DeferredActions.h>

namespace EGT::ImGui_impl {
    std::vector<std::function<void()>> DeferredActions::actions{};
    std::mutex DeferredActions::writeMutex{};
    std::shared_mutex DeferredActions::readMutex{};

    void DeferredActions::Add(const std::function<void()>& action) {
        std::lock_guard lock(writeMutex);
        actions.push_back(action);
    }
    void DeferredActions::Clear() {
        std::lock_guard lock(writeMutex);
        actions.clear();
    }

    bool DeferredActions::HasPendingActions() {
        std::shared_lock lock(readMutex);
        return !actions.empty();
    }
    void DeferredActions::Process() {
        std::vector<std::function<void()>> actionsToProcess;
        {
            std::lock_guard lock(writeMutex);
            actionsToProcess.swap(actions);
        }

        for (auto& action : actionsToProcess)
            action();
    }
}