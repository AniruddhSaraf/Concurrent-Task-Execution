#pragma once
#include "task.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TaskNode {
  private:
    std::shared_ptr<Task> task;

    // DAG edges
    std::vector<TaskNode*> dependents;   // nodes that depend on *this*
    std::atomic<int> pendingDeps;        // how many upstream deps remain

    // Completion callback (set by DAGScheduler to enqueue ready nodes)
    std::function<void(TaskNode*)> onComplete;

    mutable std::mutex nodeMutex;        // protects dependents list during construction

  public:
    explicit TaskNode(std::shared_ptr<Task> task);

    // Graph wiring (called by TaskGraph during construction)
    void addDependent(TaskNode* node);
    void incrementPendingDeps();

    // Execution (called by DAGScheduler worker threads)
    void run();

    // Query
    bool isReady() const;
    int getPendingDeps() const;
    std::string getName() const;
    int getId() const;

    // Expose dependents for graph validation (Kahn's algorithm)
    const std::vector<TaskNode*>& getDependents() const;

    // Called by DAGScheduler to inject the enqueue callback
    void setOnComplete(std::function<void(TaskNode*)> callback);
};
