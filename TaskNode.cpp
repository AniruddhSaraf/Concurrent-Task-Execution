#include "TaskNode.hpp"

TaskNode::TaskNode(std::shared_ptr<Task> t)
    : task(std::move(t)), pendingDeps(0) {}

void TaskNode::addDependent(TaskNode* node) {
    std::lock_guard<std::mutex> lock(nodeMutex);
    dependents.push_back(node);
}

void TaskNode::incrementPendingDeps() {
    pendingDeps.fetch_add(1, std::memory_order_relaxed);
}

void TaskNode::run() {
    // Execute the wrapped task
    task->execute();

    // Notify all dependents — lock-free hot path
    for (TaskNode* dep : dependents) {
        // Atomically decrement the dependent's pending count
        int prev = dep->pendingDeps.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1) {
            // This dependent just became ready (0 remaining deps).
            // Exactly one thread will see prev == 1, so no duplicate enqueue.
            if (onComplete) {
                onComplete(dep);
            }
        }
    }
}

bool TaskNode::isReady() const {
    return pendingDeps.load(std::memory_order_acquire) == 0;
}

int TaskNode::getPendingDeps() const {
    return pendingDeps.load(std::memory_order_acquire);
}

std::string TaskNode::getName() const { return task->getName(); }
int TaskNode::getId() const { return task->getId(); }

const std::vector<TaskNode*>& TaskNode::getDependents() const {
    return dependents;
}

void TaskNode::setOnComplete(std::function<void(TaskNode*)> callback) {
    onComplete = std::move(callback);
}
