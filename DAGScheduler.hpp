#pragma once
#include "TaskGraph.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class DAGScheduler {
  private:
    std::vector<std::thread> threadPool;
    std::deque<TaskNode*> readyQueue;      // nodes ready to execute
    std::mutex queueMutex;
    std::condition_variable queueCV;

    std::atomic<int> completedCount{0};    // track how many nodes finished
    int totalNodes = 0;

    std::mutex completionMutex;
    std::condition_variable completionCV;  // signals full graph completion

    bool running = true;

    void worker();

  public:
    explicit DAGScheduler(int threadCount);
    ~DAGScheduler();

    // Execute a task graph. Blocks until all tasks in the DAG complete.
    void execute(TaskGraph& graph);
};
