#!/bin/bash

# Script to run a 3-node Raft cluster for testing

set -e

echo "Starting 3-node Raft cluster..."

# Check if executables exist
if [ ! -f "build/raft_server" ]; then
    echo "Error: raft_server not found. Please run './build.sh' first."
    exit 1
fi

# Create data directory if it doesn't exist
mkdir -p data

# Kill any existing processes on these ports
echo "Cleaning up any existing processes..."
pkill -f "raft_server" || true
sleep 1

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "Stopping all Raft nodes..."
    pkill -f "raft_server" || true
    echo "Cleanup completed."
}

# Set trap to cleanup on script exit
trap cleanup EXIT INT TERM

# Start nodes in background
echo "Starting node1 on localhost:50051..."
cd build
./raft_server node1 localhost:50051 localhost:50052 localhost:50053 > ../data/node1.log 2>&1 &
NODE1_PID=$!

echo "Starting node2 on localhost:50052..."
./raft_server node2 localhost:50052 localhost:50051 localhost:50053 > ../data/node2.log 2>&1 &
NODE2_PID=$!

echo "Starting node3 on localhost:50053..."
./raft_server node3 localhost:50053 localhost:50051 localhost:50052 > ../data/node3.log 2>&1 &
NODE3_PID=$!

cd ..

echo ""
echo "All nodes started successfully!"
echo "Node1 PID: $NODE1_PID (logs: data/node1.log)"
echo "Node2 PID: $NODE2_PID (logs: data/node2.log)"
echo "Node3 PID: $NODE3_PID (logs: data/node3.log)"
echo ""
echo "Monitor cluster state with:"
echo "  tail -f data/node*.log"
echo ""
echo "Or run the test client:"
echo "  cd build && ./raft_test"
echo ""
echo "Press Ctrl+C to stop all nodes..."

# Wait for user to stop
while true; do
    sleep 1
done 