#include "DAGScheduler.hpp"
#include <iostream>

DAGScheduler::DAGScheduler(int threadCount) {
    for (int i = 0; i < threadCount; ++i) {
        threadPool.emplace_back(&DAGScheduler::worker, this);
    }
}

DAGScheduler::~DAGScheduler() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }
    queueCV.notify_all();

    for (auto& t : threadPool) {
        if (t.joinable()) t.join();
    }
}

void DAGScheduler::execute(TaskGraph& graph) {
    if (!graph.validate()) {
        std::cerr << "DAGScheduler: Invalid task graph (cycle detected). Aborting.\n";
        return;
    }

    totalNodes = static_cast<int>(graph.size());
    completedCount.store(0, std::memory_order_relaxed);

    // Inject the enqueue callback into every node.
    // When a node finishes and a dependent becomes ready (pendingDeps hits 0),
    // this callback pushes the dependent into the ready queue.
    auto enqueueReady = [this](TaskNode* node) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            readyQueue.push_back(node);
        }
        queueCV.notify_one();
    };

    for (TaskNode* node : graph.getAllNodes()) {
        node->setOnComplete(enqueueReady);
    }

    // Seed the queue with root nodes (zero dependencies)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (TaskNode* root : graph.getRootNodes()) {
            readyQueue.push_back(root);
        }
    }
    queueCV.notify_all();

    // Block until all nodes have completed
    {
        std::unique_lock<std::mutex> lock(completionMutex);
        completionCV.wait(lock, [this]() {
            return completedCount.load(std::memory_order_acquire) >= totalNodes;
        });
    }

    std::cout << "DAGScheduler: All " << totalNodes
              << " tasks completed successfully.\n";
}

void DAGScheduler::worker() {
    while (true) {
        TaskNode* node = nullptr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this]() {
                return !readyQueue.empty() || !running;
            });

            if (!running && readyQueue.empty()) return;

            node = readyQueue.front();
            readyQueue.pop_front();
        }

        // Execute the task and propagate readiness to dependents
        node->run();

        // Track completion and signal if the entire graph is done
        int finished = completedCount.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (finished >= totalNodes) {
            completionCV.notify_one();
        }
    }
}
