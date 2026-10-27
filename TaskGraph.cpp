#include "TaskGraph.hpp"
#include <iostream>
#include <queue>
#include <stdexcept>
#include <unordered_map>

TaskNode* TaskGraph::addTask(std::shared_ptr<Task> task) {
    int id = task->getId();
    auto node = std::make_unique<TaskNode>(std::move(task));
    TaskNode* ptr = node.get();
    nodeById[id] = ptr;
    nodes.push_back(std::move(node));
    return ptr;
}

void TaskGraph::addDependency(TaskNode* from, TaskNode* to) {
    // `from` must finish before `to` can start
    from->addDependent(to);
    to->incrementPendingDeps();
    validated = false;  // graph changed, must re-validate
}

bool TaskGraph::validate() {
    // Full Kahn's algorithm for cycle detection.
    // Uses a *temporary* in-degree map so the actual pendingDeps atomics
    // are not consumed — the graph remains executable after validation.

    std::unordered_map<TaskNode*, int> inDegree;

    // Initialize in-degree for every node to 0
    for (auto& node : nodes) {
        inDegree[node.get()] = 0;
    }

    // Count incoming edges by iterating each node's dependents (outgoing edges)
    for (auto& node : nodes) {
        for (TaskNode* dep : node->getDependents()) {
            inDegree[dep]++;
        }
    }

    // Seed the queue with all zero in-degree nodes
    std::queue<TaskNode*> ready;
    for (auto& [nodePtr, deg] : inDegree) {
        if (deg == 0) {
            ready.push(nodePtr);
        }
    }

    int processedCount = 0;
    while (!ready.empty()) {
        TaskNode* current = ready.front();
        ready.pop();
        processedCount++;

        for (TaskNode* dep : current->getDependents()) {
            inDegree[dep]--;
            if (inDegree[dep] == 0) {
                ready.push(dep);
            }
        }
    }

    if (processedCount != static_cast<int>(nodes.size())) {
        std::cerr << "TaskGraph ERROR: Cycle detected! Processed "
                  << processedCount << " of " << nodes.size()
                  << " nodes.\n";
        validated = false;
        return false;
    }

    validated = true;
    return true;
}

std::vector<TaskNode*> TaskGraph::getRootNodes() const {
    std::vector<TaskNode*> roots;
    for (auto& node : nodes) {
        if (node->isReady()) {
            roots.push_back(node.get());
        }
    }
    return roots;
}

std::vector<TaskNode*> TaskGraph::getAllNodes() const {
    std::vector<TaskNode*> all;
    all.reserve(nodes.size());
    for (auto& node : nodes) {
        all.push_back(node.get());
    }
    return all;
}

void TaskGraph::printGraph() const {
    std::cout << "\n=== Task Graph (" << nodes.size() << " nodes) ===\n";
    for (auto& node : nodes) {
        std::cout << "  [" << node->getId() << "] " << node->getName()
                  << "  (pending deps: " << node->getPendingDeps() << ")";
        const auto& deps = node->getDependents();
        if (!deps.empty()) {
            std::cout << "  -> {";
            for (size_t i = 0; i < deps.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << deps[i]->getName();
            }
            std::cout << "}";
        }
        std::cout << "\n";
    }
    std::cout << "================================\n\n";
}

size_t TaskGraph::size() const { return nodes.size(); }
