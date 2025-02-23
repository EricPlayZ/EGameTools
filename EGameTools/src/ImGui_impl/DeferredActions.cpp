#include <EGT\ImGui_impl\DeferredActions.h>

namespace EGT::ImGui_impl {
    std::vector<std::function<void()>> DeferredActions::actions{};
    std::mutex DeferredActions::writingMutex{};
    std::shared_mutex DeferredActions::readingMutex{};

    void DeferredActions::Add(const std::function<void()>& action) {
        std::lock_guard lock(writingMutex);
        actions.push_back(action);
    }
    void DeferredActions::Clear() {
        std::lock_guard lock(writingMutex);
        actions.clear();
    }

    bool DeferredActions::HasPendingActions() {
        std::shared_lock lock(readingMutex);
        return !actions.empty();
    }
    void DeferredActions::Process() {
        std::vector<std::function<void()>> actionsToProcess;
        {
            std::lock_guard lock(writingMutex);
            actionsToProcess.swap(actions);
        }

        for (auto& action : actionsToProcess)
            action();
    }
}