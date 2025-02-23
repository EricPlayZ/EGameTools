#include <EGT\ImGui_impl\NextFrameTask.h>

namespace EGT::ImGui_impl {
    std::vector<NextFrameTask::TaskEntry> NextFrameTask::taskQueue;
    std::mutex NextFrameTask::writingMutex{};
    std::shared_mutex NextFrameTask::readingMutex{};

    void NextFrameTask::AddTask(const Task& task, int delayFrames) {
        std::lock_guard lock(writingMutex);
        taskQueue.emplace_back(TaskEntry{ delayFrames, task });
    }
    void NextFrameTask::ExecuteTasks() {
        std::lock_guard lock(writingMutex);
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