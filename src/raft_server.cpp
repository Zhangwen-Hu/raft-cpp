#include "raft_server.h"
#include <iostream>

namespace raft {

RaftServer::RaftServer(std::unique_ptr<RaftNode> node, const std::string& server_address)
    : raft_node_(std::move(node))
    , server_address_(server_address)
    , service_impl_(std::make_unique<RaftServiceImpl>(raft_node_.get()))
{
}

RaftServer::~RaftServer() {
    stop();
}

void RaftServer::start() {
    if (!raft_node_) {
        std::cerr << "Error: RaftNode is null" << std::endl;
        return;
    }
    
    // Start the Raft node
    raft_node_->start();
    
    std::cout << "RaftServer started on " << server_address_ << std::endl;
    
    // TODO: Initialize and start gRPC server once protobuf is generated
    /*
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_impl_.get());
    
    server_ = builder.BuildAndStart();
    */
}

void RaftServer::stop() {
    if (raft_node_) {
        raft_node_->stop();
    }
    
    // TODO: Stop gRPC server once it's implemented
    /*
    if (server_) {
        server_->Shutdown();
    }
    */
    
    std::cout << "RaftServer stopped" << std::endl;
}

void RaftServer::wait() {
    // TODO: Wait for gRPC server once it's implemented
    /*
    if (server_) {
        server_->Wait();
    }
    */
    
    // For now, just wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// RaftServiceImpl implementation

RaftServiceImpl::RaftServiceImpl(RaftNode* raft_node)
    : raft_node_(raft_node)
{
}

// TODO: Implement actual gRPC service methods once protobuf is generated
/*
grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context,
                                         const raft::RequestVoteRequest* request,
                                         raft::RequestVoteResponse* response) {
    raft_node_->handleRequestVote(request, response);
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context,
                                           const raft::AppendEntriesRequest* request,
                                           raft::AppendEntriesResponse* response) {
    raft_node_->handleAppendEntries(request, response);
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::InstallSnapshot(grpc::ServerContext* context,
                                             const raft::InstallSnapshotRequest* request,
                                             raft::InstallSnapshotResponse* response) {
    raft_node_->handleInstallSnapshot(request, response);
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::SubmitCommand(grpc::ServerContext* context,
                                           const raft::ClientRequest* request,
                                           raft::ClientResponse* response) {
    std::vector<uint8_t> command(request->command().begin(), request->command().end());
    bool success = raft_node_->submitCommand(command);
    
    response->set_success(success);
    if (!success) {
        response->set_leader_hint(raft_node_->getLeaderId());
    }
    
    return grpc::Status::OK;
}
*/

} // namespace raft 