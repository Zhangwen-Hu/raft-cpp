#pragma once

#include "log_entry.h"
#include "persistent_state.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

// Forward declarations for protobuf types (will be available after generation)
namespace raft {
    class RequestVoteRequest;
    class RequestVoteResponse;
    class AppendEntriesRequest;
    class AppendEntriesResponse;
    class InstallSnapshotRequest;
    class InstallSnapshotResponse;
    class RaftService;
}

namespace raft {

enum class NodeState {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

struct PeerInfo {
    std::string id;
    std::string address;
    void* stub; // Will be replaced with proper gRPC stub type later
    
    // Leader state (only used by leader)
    int64_t next_index = 1;
    int64_t match_index = 0;
};

class RaftNode {
public:
    RaftNode(const std::string& node_id, 
             const std::string& address,
             const std::vector<std::string>& peer_addresses);
    
    ~RaftNode();
    
    // Start the Raft node
    void start();
    void stop();
    
    // RPC handlers
    void handleRequestVote(const raft::RequestVoteRequest* request,
                          raft::RequestVoteResponse* response);
    
    void handleAppendEntries(const raft::AppendEntriesRequest* request,
                            raft::AppendEntriesResponse* response);
    
    void handleInstallSnapshot(const raft::InstallSnapshotRequest* request,
                              raft::InstallSnapshotResponse* response);
    
    // Client interface
    bool submitCommand(const std::vector<uint8_t>& command);
    
    // Status queries
    bool isLeader() const;
    std::string getLeaderId() const;
    NodeState getState() const;
    int64_t getCurrentTerm() const;

private:
    // Node identification
    std::string node_id_;
    std::string address_;
    
    // Persistent state
    std::unique_ptr<PersistentState> persistent_state_;
    Log log_;
    
    // Volatile state (all servers)
    mutable std::mutex state_mutex_;
    std::atomic<NodeState> state_;
    int64_t commit_index_ = 0;
    int64_t last_applied_ = 0;
    std::string current_leader_id_;
    
    // Volatile state (leaders only)
    std::map<std::string, PeerInfo> peers_;
    
    // Timing
    std::chrono::steady_clock::time_point last_heartbeat_;
    std::atomic<bool> election_timeout_reset_{false};
    
    // Background threads  
    std::atomic<bool> running_{false};
    std::thread election_thread_;
    std::thread heartbeat_thread_;
    std::thread apply_thread_;
    
    // Random number generation for election timeout
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> timeout_dist_;
    
    // Configuration
    static constexpr int HEARTBEAT_INTERVAL_MS = 50;
    static constexpr int ELECTION_TIMEOUT_MIN_MS = 150;
    static constexpr int ELECTION_TIMEOUT_MAX_MS = 300;
    
    // Core Raft algorithm methods
    void becomeFollower(int64_t term);
    void becomeCandidate();
    void becomeLeader();
    
    // Election
    void startElection();
    bool requestVoteFromPeer(const std::string& peer_id);
    int getElectionTimeout();
    
    // Heartbeats and log replication
    void sendHeartbeats();
    bool sendAppendEntriesToPeer(const std::string& peer_id);
    void updateCommitIndex();
    
    // Log application
    void applyCommittedEntries();
    
    // Background threads
    void electionLoop();
    void heartbeatLoop();
    void applyLoop();
    
    // Utility methods
    void resetElectionTimeout();
    bool isElectionTimeoutExpired() const;
    void initializePeers(const std::vector<std::string>& peer_addresses);
    int64_t getLastLogIndex() const;
    int64_t getLastLogTerm() const;
    bool isLogUpToDate(int64_t last_log_index, int64_t last_log_term) const;
};

} // namespace raft 