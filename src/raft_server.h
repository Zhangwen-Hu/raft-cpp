#pragma once

#include "raft_node.h"
#include <memory>
#include <string>

// Forward declarations (will be available after protobuf generation)
namespace grpc {
    class Server;
    class ServerBuilder;
}

namespace raft {
    class RaftService;
}

namespace raft {

class RaftServiceImpl; // Forward declaration

class RaftServer {
public:
    RaftServer(std::unique_ptr<RaftNode> node, const std::string& server_address);
    ~RaftServer();
    
    // Start and stop the gRPC server
    void start();
    void stop();
    
    // Wait for server to shutdown
    void wait();
    
    // Get the underlying Raft node
    RaftNode* getRaftNode() const { return raft_node_.get(); }

private:
    std::unique_ptr<RaftNode> raft_node_;
    std::string server_address_;
    std::unique_ptr<RaftServiceImpl> service_impl_;
    std::unique_ptr<grpc::Server> server_;
};

// gRPC service implementation
class RaftServiceImpl {
public:
    explicit RaftServiceImpl(RaftNode* raft_node);
    
    // Will implement the actual gRPC service methods once protobuf is generated
    // For now, keep a reference to the raft node
    RaftNode* raft_node_;
};

} // namespace raft 