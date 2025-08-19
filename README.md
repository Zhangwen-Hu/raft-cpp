# Raft-CPP

A complete implementation of the Raft consensus algorithm in C++ with gRPC for distributed systems.

## Overview

This project implements the Raft consensus algorithm as described in the paper "In Search of an Understandable Consensus Algorithm" by Diego Ongaro and John Ousterhout. The implementation includes:

- **Leader Election**: Automatic leader election with randomized timeouts
- **Log Replication**: Reliable log replication across cluster nodes  
- **Safety Guarantees**: Ensures strong consistency and safety properties
- **gRPC Communication**: High-performance inter-node communication
- **Persistent State**: Crash-recovery with persistent storage
- **Client Interface**: Simple API for submitting commands

## Architecture

The implementation consists of several key components:

### Core Components

1. **RaftNode** (`src/raft_node.h/cpp`): Core Raft algorithm implementation
2. **Log** (`src/log_entry.h/cpp`): Log entry management and operations
3. **PersistentState** (`src/persistent_state.h/cpp`): Persistent storage for critical state
4. **RaftServer** (`src/raft_server.h/cpp`): gRPC server wrapper

### Raft Algorithm Features

- **Three Node States**: Follower, Candidate, Leader
- **Term-based Leadership**: Monotonically increasing terms
- **Majority Consensus**: Requires majority votes for leadership and commits
- **Log Consistency**: Ensures identical logs across committed entries
- **Network Partition Tolerance**: Handles network failures gracefully

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- Protocol Buffers 3.0+
- gRPC 1.16+
- pthread (for threading support)

### Install Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    libgrpc-dev
```

#### macOS (with Homebrew)
```bash
brew install cmake protobuf grpc
```

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd raft-cpp

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)
```

### Build Outputs

- `raft_server`: Main Raft server executable
- `raft_test`: Test cluster application
- `libraft_lib.a`: Static library for integration

## Usage

### Running a Single Node

```bash
./raft_server <node_id> <listen_address> [peer_address1] [peer_address2] ...
```

Example:
```bash
./raft_server node1 localhost:50051 localhost:50052 localhost:50053
```

### Running a Test Cluster

The test cluster automatically creates a 3-node cluster for demonstration:

```bash
./raft_test
```

This will:
1. Start 3 Raft nodes (node1, node2, node3)
2. Monitor leader election process
3. Submit test commands through the leader
4. Display cluster state for 30 seconds
5. Cleanly shutdown all nodes

### Multi-Node Cluster Setup

To run a 3-node cluster manually:

**Terminal 1 (Node 1):**
```bash
./raft_server node1 localhost:50051 localhost:50052 localhost:50053
```

**Terminal 2 (Node 2):**
```bash
./raft_server node2 localhost:50052 localhost:50051 localhost:50053
```

**Terminal 3 (Node 3):**
```bash
./raft_server node3 localhost:50053 localhost:50051 localhost:50052
```

## API Reference

### RaftNode Interface

```cpp
class RaftNode {
public:
    // Constructor
    RaftNode(const std::string& node_id, 
             const std::string& address,
             const std::vector<std::string>& peer_addresses);
    
    // Lifecycle
    void start();
    void stop();
    
    // Client interface
    bool submitCommand(const std::vector<uint8_t>& command);
    
    // Status queries
    bool isLeader() const;
    std::string getLeaderId() const;
    NodeState getState() const;
    int64_t getCurrentTerm() const;
};
```

### gRPC Service Definition

The Raft protocol uses the following gRPC service:

```protobuf
service RaftService {
    rpc RequestVote(RequestVoteRequest) returns (RequestVoteResponse);
    rpc AppendEntries(AppendEntriesRequest) returns (AppendEntriesResponse);
    rpc InstallSnapshot(InstallSnapshotRequest) returns (InstallSnapshotResponse);
    rpc SubmitCommand(ClientRequest) returns (ClientResponse);
}
```

## Configuration Parameters

The implementation uses the following timing parameters:

- **Heartbeat Interval**: 50ms (configurable in `raft_node.h`)
- **Election Timeout**: 150-300ms (randomized)
- **RPC Timeout**: 5 seconds (configurable per RPC)

## File Structure

```
raft-cpp/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── proto/
│   └── raft.proto         # Protocol buffer definitions
└── src/
    ├── log_entry.h/cpp    # Log entry management
    ├── persistent_state.h/cpp # Persistent state storage
    ├── raft_node.h/cpp    # Core Raft implementation
    ├── raft_server.h/cpp  # gRPC server wrapper
    ├── main.cpp           # Main server application
    └── test_cluster.cpp   # Test cluster application
```

## Testing

### Unit Testing

While comprehensive unit tests are not included in this initial implementation, the design supports easy testing of individual components:

- `Log` class for log operations
- `PersistentState` for state persistence
- `RaftNode` for algorithm logic

### Integration Testing

Use the provided `raft_test` executable to verify:

1. Leader election in various scenarios
2. Log replication across nodes
3. Network partition handling
4. Recovery from failures

### Manual Testing Scenarios

1. **Leader Election**: Start nodes sequentially and observe leader election
2. **Log Replication**: Submit commands and verify they replicate to all nodes
3. **Leader Failure**: Kill leader and observe new leader election
4. **Network Partition**: Use firewall rules to simulate network partitions
5. **Recovery**: Restart failed nodes and verify they catch up

## Limitations and Future Work

### Current Limitations

1. **Snapshot Support**: Basic snapshot interface exists but needs full implementation
2. **Dynamic Membership**: Cluster membership changes not yet implemented
3. **Persistence Optimization**: Log compaction and cleanup not implemented
4. **Production Hardening**: Additional error handling and monitoring needed

### Future Enhancements

- [ ] Complete snapshot implementation
- [ ] Dynamic cluster membership changes
- [ ] Log compaction and cleanup
- [ ] Metrics and monitoring integration
- [ ] Comprehensive test suite
- [ ] Performance optimizations
- [ ] Configuration file support

## Performance Considerations

- **Throughput**: Optimized for consistency over raw throughput
- **Latency**: Commit latency is 1-2 network round-trips
- **Memory Usage**: Grows with log size (compaction needed for production)
- **CPU Usage**: Background threads for election and heartbeat management

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## References

1. [Raft Paper](https://raft.github.io/raft.pdf) - Original Raft consensus algorithm paper
2. [Raft Website](https://raft.github.io/) - Interactive visualization and resources
3. [gRPC Documentation](https://grpc.io/docs/) - gRPC framework documentation
4. [Protocol Buffers](https://developers.google.com/protocol-buffers) - Serialization format

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

### Development Guidelines

1. Follow the existing code style and structure
2. Add appropriate comments and documentation
3. Test thoroughly before submitting
4. Update README.md for new features