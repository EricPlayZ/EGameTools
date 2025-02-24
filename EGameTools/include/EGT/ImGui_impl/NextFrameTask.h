#pragma once
#include <functional>
#include <vector>
#include <mutex>
#include <shared_mutex>

namespace EGT::ImGui_impl {
    class NextFrameTask {
        using Task = std::function<void()>;

    public:
        static void AddTask(const Task& task, int delayFrames = 1);
        static void ExecuteTasks();
    private:
        struct TaskEntry {
            int framesRemaining = 1;
            Task task{};
        };

        static std::vector<TaskEntry> taskQueue;
        static std::mutex writeMutex;
        static std::shared_mutex readMutex;
    };
}