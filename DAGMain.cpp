#include "DAGScheduler.hpp"
#include "TaskGraph.hpp"
#include "task.hpp"
#include <chrono>
#include <iostream>
#include <thread>

// ─── Concrete task that simulates variable-duration GPU/CPU work ───
struct ComputeTask : public Task {
    int durationMs;

    ComputeTask(const std::string& name, int ms)
        : Task(name), durationMs(ms) {}

    void execute() override {
        std::cout << "[START] " << getName()
                  << "  (thread " << std::this_thread::get_id() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        std::cout << "[DONE]  " << getName()
                  << "  (thread " << std::this_thread::get_id() << ")\n";
    }
};

int main() {
    /*
     * DAG Structure (AI inference pipeline):
     *
     *     A (Load Data, 100ms)      B (Init Model, 150ms)
     *           \                        /
     *            v                      v
     *             C (Preprocess, 200ms)
     *            / \
     *           v   v
     *   D (Inference-GPU0, 50ms)  E (Inference-GPU1, 80ms)
     *           \                /
     *            v              v
     *          F (Merge Results, 100ms)
     *
     * Critical path: B(150) -> C(200) -> E(80) -> F(100) = 530 ms
     * Sequential sum: 100+150+200+50+80+100 = 680 ms
     */

    TaskGraph graph;

    // Create tasks with realistic GPU pipeline names
    auto taskA = std::make_shared<ComputeTask>("Load-Data",       100);
    auto taskB = std::make_shared<ComputeTask>("Init-Model",      150);
    auto taskC = std::make_shared<ComputeTask>("Preprocess",      200);
    auto taskD = std::make_shared<ComputeTask>("Inference-GPU0",   50);
    auto taskE = std::make_shared<ComputeTask>("Inference-GPU1",   80);
    auto taskF = std::make_shared<ComputeTask>("Merge-Results",   100);

    // Register nodes in the graph
    TaskNode* nA = graph.addTask(taskA);
    TaskNode* nB = graph.addTask(taskB);
    TaskNode* nC = graph.addTask(taskC);
    TaskNode* nD = graph.addTask(taskD);
    TaskNode* nE = graph.addTask(taskE);
    TaskNode* nF = graph.addTask(taskF);

    // Wire dependencies (edges in the DAG)
    graph.addDependency(nA, nC);  // A -> C
    graph.addDependency(nB, nC);  // B -> C
    graph.addDependency(nC, nD);  // C -> D
    graph.addDependency(nC, nE);  // C -> E
    graph.addDependency(nD, nF);  // D -> F
    graph.addDependency(nE, nF);  // E -> F

    // Print the graph structure
    graph.printGraph();

    // Execute with 4 worker threads
    DAGScheduler scheduler(4);

    auto start = std::chrono::high_resolution_clock::now();
    scheduler.execute(graph);
    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\n--- Performance Summary ---\n";
    std::cout << "Wall time (DAG):    " << elapsed.count() << " ms\n";
    std::cout << "Critical path:      ~530 ms  (B->C->E->F)\n";
    std::cout << "Sequential total:   680 ms  (sum of all tasks)\n";
    std::cout << "Speedup:            "
              << (680.0 / elapsed.count()) << "x\n";

    return 0;
}
