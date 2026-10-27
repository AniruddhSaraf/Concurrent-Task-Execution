#pragma once
#include "TaskNode.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TaskGraph {
  private:
    std::vector<std::unique_ptr<TaskNode>> nodes;
    std::unordered_map<int, TaskNode*> nodeById;  // taskId -> TaskNode*

    bool validated = false;

  public:
    // Add a task to the graph; returns the TaskNode* for wiring dependencies
    TaskNode* addTask(std::shared_ptr<Task> task);

    // Declare: `from` must complete before `to` can run
    void addDependency(TaskNode* from, TaskNode* to);

    // Validate the graph is a valid DAG (no cycles) using Kahn's algorithm.
    // Returns true if valid.
    bool validate();

    // Get all nodes with zero pending dependencies (entry points)
    std::vector<TaskNode*> getRootNodes() const;

    // Get all nodes (for DAGScheduler to set callbacks)
    std::vector<TaskNode*> getAllNodes() const;

    // Print the graph structure to stdout
    void printGraph() const;

    // Number of nodes in the graph
    size_t size() const;
};
