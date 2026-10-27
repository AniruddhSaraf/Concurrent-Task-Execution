
# Concurrent Task Execution

A C++ framework for concurrent task execution featuring two scheduling modes: a **FIFO Thread Pool Scheduler** and a **DAG-based Task Graph Scheduler**. The project demonstrates practical use of threads, mutexes, condition variables, atomics, and lock-free patterns in modern C++.

## Features

- **Task Creation and Execution:** Define tasks with specific functionalities that can be executed concurrently.
- **Dynamic Task Scheduling:** Tasks are queued and scheduled dynamically across multiple threads for execution.
- **Thread Pool Management:** Utilizes a pool of threads to execute tasks concurrently, optimizing resource use and execution time.
- **Concurrency Control:** Ensures thread safety through mutexes and condition variables, managing access to the task queue.
- **DAG Task Graph Scheduling:** Define task dependencies as a Directed Acyclic Graph (DAG). Tasks execute only after all their upstream dependencies complete, enabling complex pipeline parallelism.
- **Cycle Detection:** Validates task graphs using Kahn's algorithm before execution to reject cyclic dependencies.
- **Lock-Free Dependency Resolution:** Uses `std::atomic` fetch-and-subtract for dependency propagation on the hot path, minimizing lock contention.

## Architecture

The project is organized into two layers:

```
┌─────────────────────────────────────────────────────┐
│              DAG Layer (new)                         │
│  DAGScheduler ──▶ TaskGraph ──▶ TaskNode             │
│  (executes)       (validates)    (wraps Task +       │
│                                   tracks deps)       │
├─────────────────────────────────────────────────────┤
│              Core Layer (original)                   │
│  TaskScheduler ──▶ Task (abstract base class)        │
│  (FIFO thread pool)                                  │
└─────────────────────────────────────────────────────┘
```

## Getting Started

### Prerequisites

- A modern C++ compiler that supports C++17 (or later).
- Basic understanding of C++ threading and object-oriented programming.

### File Structure

**Core (original):**
- `task.hpp/cpp`: Defines the abstract `Task` class for task creation and management.
- `TaskScheduler.hpp/cpp`: Implements the FIFO Task Scheduler, managing the task queue and thread pool.
- `Main.cpp`: Demonstrates the usage of the FIFO Task Scheduler with example tasks.
- `compile.sh`: Shell script to compile the FIFO scheduler demo.

**DAG Extension:**
- `TaskNode.hpp/cpp`: Wraps a `Task` with dependency metadata (`pendingDeps`, `dependents` list) and lock-free ready propagation.
- `TaskGraph.hpp/cpp`: DAG container with `addDependency()`, cycle detection via Kahn's algorithm, and root-node discovery.
- `DAGScheduler.hpp/cpp`: Thread-pool executor that respects DAG ordering — seeds root tasks, dynamically enqueues dependents upon completion.
- `DAGMain.cpp`: Demonstrates a 6-node diamond DAG simulating a GPU inference pipeline.
- `compile_dag.sh`: Shell script to compile the DAG scheduler demo.

### Compilation and Execution

#### FIFO Scheduler (original)

```bash
# Compile
./compile.sh

# Run
./Executable
```

#### DAG Scheduler

```bash
# Compile
./compile_dag.sh

# Run
./DAGExecutable
```

Or compile manually:

```bash
g++ -std=c++17 -Wall -Wextra DAGMain.cpp task.cpp TaskNode.cpp TaskGraph.cpp DAGScheduler.cpp -lpthread -o DAGExecutable
```

## Usage

### FIFO Scheduler

Define your tasks by extending the `Task` class and implementing the `execute()` method. Add tasks to the scheduler using `addTask(std::unique_ptr<Task> task)`. The scheduler automatically handles the execution of tasks across its thread pool.

```cpp
struct MyTask : public Task {
    MyTask(const std::string& name) : Task(name) {}
    void execute() override {
        std::cout << getName() << " executed.\n";
    }
};

TaskScheduler scheduler(4);  // 4 worker threads
scheduler.addTask(std::make_unique<MyTask>("Task-1"));
```

### DAG Scheduler

Define tasks, register them in a `TaskGraph`, wire dependencies with `addDependency()`, and submit the graph to a `DAGScheduler`:

```cpp
TaskGraph graph;

auto loadData   = std::make_shared<ComputeTask>("Load-Data",   100);
auto preprocess = std::make_shared<ComputeTask>("Preprocess",  200);
auto inference  = std::make_shared<ComputeTask>("Inference",    50);

TaskNode* nLoad = graph.addTask(loadData);
TaskNode* nPrep = graph.addTask(preprocess);
TaskNode* nInf  = graph.addTask(inference);

graph.addDependency(nLoad, nPrep);  // Load -> Preprocess
graph.addDependency(nPrep, nInf);   // Preprocess -> Inference

DAGScheduler scheduler(4);
scheduler.execute(graph);  // blocks until all tasks complete
```

### Example DAG (from `DAGMain.cpp`)

```
    A (Load Data)     B (Init Model)
         \                 /
          v               v
           C (Preprocess)
          / \
         v   v
D (Inference-GPU0)  E (Inference-GPU1)
         \               /
          v             v
        F (Merge Results)
```

**Sample output:**
```
[START] Load-Data    (thread 2)
[START] Init-Model   (thread 3)
[DONE]  Load-Data    (thread 2)
[DONE]  Init-Model   (thread 3)
[START] Preprocess   (thread 3)
[DONE]  Preprocess   (thread 3)
[START] Inference-GPU0  (thread 3)
[START] Inference-GPU1  (thread 5)
[DONE]  Inference-GPU0  (thread 3)
[DONE]  Inference-GPU1  (thread 5)
[START] Merge-Results   (thread 4)
[DONE]  Merge-Results   (thread 4)
DAGScheduler: All 6 tasks completed successfully.

Wall time (DAG):    ~567 ms
Critical path:      ~530 ms  (B->C->E->F)
Sequential total:   680 ms
Speedup:            ~1.2x
```

## Implementation Notes

- Ensure all source files are in the same directory. The `TaskScheduler` relies on `task.hpp/cpp`, and the DAG layer relies on all core files plus the new `TaskNode`, `TaskGraph`, and `DAGScheduler` files.
- The `Task` class is designed to be extended for specific tasks. Each task must implement the `execute()` method, defining the task's behavior.
- The `TaskScheduler` class manages a pool of worker threads that execute tasks from a FIFO queue. It demonstrates basic concurrency control mechanisms, such as mutexes and condition variables, to safely manage access to the shared task queue.
- The `DAGScheduler` uses a callback-driven approach: when a task completes, it atomically decrements the `pendingDeps` counter of each dependent. When a dependent's counter reaches zero, the callback enqueues it into the ready queue. This `fetch_sub` pattern is lock-free on the critical path.
- `TaskGraph::validate()` uses Kahn's algorithm with a temporary in-degree map, so the actual `pendingDeps` atomics are not consumed during validation — the graph remains executable after validation.

## Contribution

Contributions to improve the Task Scheduler or extend its functionality are welcome. Please feel free to fork the repository, make changes, and submit a pull request.
