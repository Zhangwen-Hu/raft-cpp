#!/bin/bash

set -e

echo "Building Raft-CPP..."

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo "Running CMake..."
cmake ..

echo "Building project..."
make -j$(nproc)

echo ""
echo "Build completed successfully!"
echo ""
echo "Executables built:"
echo "  - raft_server: Main Raft server"
echo "  - raft_test: Test cluster application"
echo ""
echo "To run the test cluster:"
echo "  ./raft_test"
echo ""
echo "To run individual nodes:"
echo "  ./raft_server node1 localhost:50051 localhost:50052 localhost:50053"
echo "  ./raft_server node2 localhost:50052 localhost:50051 localhost:50053"
echo "  ./raft_server node3 localhost:50053 localhost:50051 localhost:50052" 