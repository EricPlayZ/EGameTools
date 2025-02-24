#include <EGT\ImGui_impl\NextFrameTask.h>

namespace EGT::ImGui_impl {
    std::vector<NextFrameTask::TaskEntry> NextFrameTask::taskQueue;
    std::mutex NextFrameTask::writeMutex{};
    std::shared_mutex NextFrameTask::readMutex{};

    void NextFrameTask::AddTask(const Task& task, int delayFrames) {
        std::lock_guard lock(writeMutex);
        taskQueue.emplace_back(TaskEntry{ delayFrames, task });
    }
    void NextFrameTask::ExecuteTasks() {
        std::lock_guard lock(writeMutex);
        for (auto it = taskQueue.begin(); it != taskQueue.end();) {
            if (it->framesRemaining <= 0) {
                it->task();
                it = taskQueue.erase(it);
            } else {
                --it->framesRemaining;
                ++it;
            }
        }
    }
}