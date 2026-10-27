#!/bin/bash

# Compile the DAG Task Scheduler demo
g++ -std=c++17 -Wall -Wextra \
    DAGMain.cpp task.cpp TaskNode.cpp TaskGraph.cpp DAGScheduler.cpp \
    -lpthread -o DAGExecutable

echo "Build complete: ./DAGExecutable"
