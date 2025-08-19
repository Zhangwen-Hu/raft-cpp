#include "raft_node.h"
#include <iostream>
#include <algorithm>
#include <cassert>

namespace raft {

RaftNode::RaftNode(const std::string& node_id, 
                   const std::string& address,
                   const std::vector<std::string>& peer_addresses)
    : node_id_(node_id)
    , address_(address)
    , persistent_state_(std::make_unique<PersistentState>(node_id + "_state.dat"))
    , state_(NodeState::FOLLOWER)
    , gen_(rd_())
    , timeout_dist_(ELECTION_TIMEOUT_MIN_MS, ELECTION_TIMEOUT_MAX_MS)
{
    initializePeers(peer_addresses);
    resetElectionTimeout();
}

RaftNode::~RaftNode() {
    stop();
}

void RaftNode::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (running_) {
        return;
    }
    
    running_ = true;
    
    // Start background threads
    election_thread_ = std::thread(&RaftNode::electionLoop, this);
    heartbeat_thread_ = std::thread(&RaftNode::heartbeatLoop, this);
    apply_thread_ = std::thread(&RaftNode::applyLoop, this);
    
    std::cout << "RaftNode " << node_id_ << " started" << std::endl;
}

void RaftNode::stop() {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }
    
    // Join background threads
    if (election_thread_.joinable()) {
        election_thread_.join();
    }
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
    if (apply_thread_.joinable()) {
        apply_thread_.join();
    }
    
    std::cout << "RaftNode " << node_id_ << " stopped" << std::endl;
}

void RaftNode::handleRequestVote(const raft::RequestVoteRequest* request,
                                raft::RequestVoteResponse* response) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    response->set_term(persistent_state_->getCurrentTerm());
    response->set_vote_granted(false);
    
    // Reply false if term < currentTerm
    if (request->term() < persistent_state_->getCurrentTerm()) {
        return;
    }
    
    // If RPC request contains term > currentTerm, set currentTerm = term, convert to follower
    if (request->term() > persistent_state_->getCurrentTerm()) {
        becomeFollower(request->term());
        response->set_term(persistent_state_->getCurrentTerm());
    }
    
    // If votedFor is null or candidateId, and candidate's log is at least as up-to-date as receiver's log, grant vote
    bool can_vote = !persistent_state_->hasVotedFor() || 
                    persistent_state_->getVotedFor() == request->candidate_id();
    
    bool log_up_to_date = isLogUpToDate(request->last_log_index(), request->last_log_term());
    
    if (can_vote && log_up_to_date) {
        persistent_state_->setVotedFor(request->candidate_id());
        response->set_vote_granted(true);
        resetElectionTimeout();
        
        std::cout << "Node " << node_id_ << " voted for " << request->candidate_id() 
                  << " in term " << request->term() << std::endl;
    }
}

void RaftNode::handleAppendEntries(const raft::AppendEntriesRequest* request,
                                  raft::AppendEntriesResponse* response) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    response->set_term(persistent_state_->getCurrentTerm());
    response->set_success(false);
    
    // Reply false if term < currentTerm
    if (request->term() < persistent_state_->getCurrentTerm()) {
        return;
    }
    
    // If RPC request contains term > currentTerm, set currentTerm = term, convert to follower
    if (request->term() > persistent_state_->getCurrentTerm()) {
        becomeFollower(request->term());
        response->set_term(persistent_state_->getCurrentTerm());
    }
    
    // Reset election timeout - we heard from current leader
    resetElectionTimeout();
    current_leader_id_ = request->leader_id();
    
    // If we're not a follower, become one
    if (state_ != NodeState::FOLLOWER) {
        becomeFollower(request->term());
    }
    
    // Reply false if log doesn't contain an entry at prevLogIndex whose term matches prevLogTerm
    if (request->prev_log_index() > 0) {
        if (request->prev_log_index() > getLastLogIndex() ||
            log_.getTermAt(request->prev_log_index()) != request->prev_log_term()) {
            response->set_next_index(getLastLogIndex() + 1);
            return;
        }
    }
    
    // If an existing entry conflicts with a new one (same index but different terms),
    // delete the existing entry and all that follow it
    for (int i = 0; i < request->entries_size(); ++i) {
        const auto& entry = request->entries(i);
        int64_t entry_index = request->prev_log_index() + 1 + i;
        
        if (entry_index <= getLastLogIndex()) {
            if (log_.getTermAt(entry_index) != entry.term()) {
                // Conflict detected, remove entries from this index onwards
                log_.removeEntriesFrom(entry_index);
                break;
            }
        }
    }
    
    // Append any new entries not already in the log
    for (int i = 0; i < request->entries_size(); ++i) {
        const auto& entry = request->entries(i);
        int64_t entry_index = request->prev_log_index() + 1 + i;
        
        if (entry_index > getLastLogIndex()) {
            // Convert protobuf entry to our LogEntry
            std::vector<uint8_t> command(entry.command().begin(), entry.command().end());
            LogEntry log_entry(entry.term(), entry_index, command);
            log_.append(log_entry);
        }
    }
    
    // If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
    if (request->leader_commit() > commit_index_) {
        commit_index_ = std::min(request->leader_commit(), getLastLogIndex());
    }
    
    response->set_success(true);
    response->set_next_index(getLastLogIndex() + 1);
}

void RaftNode::handleInstallSnapshot(const raft::InstallSnapshotRequest* request,
                                    raft::InstallSnapshotResponse* response) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    response->set_term(persistent_state_->getCurrentTerm());
    
    // Reply immediately if term < currentTerm
    if (request->term() < persistent_state_->getCurrentTerm()) {
        return;
    }
    
    // If RPC request contains term >= currentTerm, set currentTerm = term, convert to follower
    if (request->term() >= persistent_state_->getCurrentTerm()) {
        becomeFollower(request->term());
        response->set_term(persistent_state_->getCurrentTerm());
    }
    
    resetElectionTimeout();
    current_leader_id_ = request->leader_id();
    
    // TODO: Implement snapshot installation
    // For now, just acknowledge
    std::cout << "InstallSnapshot not fully implemented yet" << std::endl;
}

bool RaftNode::submitCommand(const std::vector<uint8_t>& command) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != NodeState::LEADER) {
        return false;
    }
    
    // Append command to local log
    int64_t new_index = getLastLogIndex() + 1;
    LogEntry entry(persistent_state_->getCurrentTerm(), new_index, command);
    log_.append(entry);
    
    std::cout << "Leader " << node_id_ << " appended command at index " << new_index << std::endl;
    
    // The command will be replicated to followers by the heartbeat thread
    return true;
}

bool RaftNode::isLeader() const {
    return state_ == NodeState::LEADER;
}

std::string RaftNode::getLeaderId() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_leader_id_;
}

NodeState RaftNode::getState() const {
    return state_;
}

int64_t RaftNode::getCurrentTerm() const {
    return persistent_state_->getCurrentTerm();
}

// Private methods

void RaftNode::becomeFollower(int64_t term) {
    state_ = NodeState::FOLLOWER;
    persistent_state_->updateTermAndVote(term, "");
    current_leader_id_.clear();
    resetElectionTimeout();
    
    std::cout << "Node " << node_id_ << " became follower in term " << term << std::endl;
}

void RaftNode::becomeCandidate() {
    state_ = NodeState::CANDIDATE;
    int64_t new_term = persistent_state_->getCurrentTerm() + 1;
    persistent_state_->updateTermAndVote(new_term, node_id_);
    current_leader_id_.clear();
    resetElectionTimeout();
    
    std::cout << "Node " << node_id_ << " became candidate in term " << new_term << std::endl;
}

void RaftNode::becomeLeader() {
    state_ = NodeState::LEADER;
    current_leader_id_ = node_id_;
    
    // Initialize leader state
    for (auto& [peer_id, peer_info] : peers_) {
        peer_info.next_index = getLastLogIndex() + 1;
        peer_info.match_index = 0;
    }
    
    std::cout << "Node " << node_id_ << " became leader in term " 
              << persistent_state_->getCurrentTerm() << std::endl;
    
    // Send initial heartbeat
    sendHeartbeats();
}

void RaftNode::startElection() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (state_ != NodeState::FOLLOWER && state_ != NodeState::CANDIDATE) {
        return;
    }
    
    becomeCandidate();
    
    int votes_received = 1; // Vote for self
    int votes_needed = (peers_.size() + 1) / 2 + 1; // Majority
    
    std::cout << "Node " << node_id_ << " starting election, need " << votes_needed 
              << " votes" << std::endl;
    
    // Request votes from all peers
    for (const auto& [peer_id, peer_info] : peers_) {
        if (requestVoteFromPeer(peer_id)) {
            votes_received++;
        }
    }
    
    // Check if we won the election
    if (votes_received >= votes_needed) {
        becomeLeader();
    }
}

bool RaftNode::requestVoteFromPeer(const std::string& peer_id) {
    // TODO: Implement actual RPC call
    // For now, simulate a vote response
    std::cout << "Requesting vote from " << peer_id << " (simulated)" << std::endl;
    return false; // Simulate rejection for now
}

int RaftNode::getElectionTimeout() {
    return timeout_dist_(gen_);
}

void RaftNode::sendHeartbeats() {
    if (state_ != NodeState::LEADER) {
        return;
    }
    
    for (const auto& [peer_id, peer_info] : peers_) {
        sendAppendEntriesToPeer(peer_id);
    }
}

bool RaftNode::sendAppendEntriesToPeer(const std::string& peer_id) {
    // TODO: Implement actual RPC call
    // For now, just log the attempt
    std::cout << "Sending AppendEntries to " << peer_id << " (simulated)" << std::endl;
    return true;
}

void RaftNode::updateCommitIndex() {
    if (state_ != NodeState::LEADER) {
        return;
    }
    
    // Find the highest index that is replicated on a majority of servers
    for (int64_t index = commit_index_ + 1; index <= getLastLogIndex(); ++index) {
        int replicas = 1; // Count self
        
        for (const auto& [peer_id, peer_info] : peers_) {
            if (peer_info.match_index >= index) {
                replicas++;
            }
        }
        
        int majority = (peers_.size() + 1) / 2 + 1;
        if (replicas >= majority && log_.getTermAt(index) == persistent_state_->getCurrentTerm()) {
            commit_index_ = index;
        }
    }
}

void RaftNode::applyCommittedEntries() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    while (last_applied_ < commit_index_) {
        last_applied_++;
        
        // TODO: Apply command to state machine
        const auto& entry = log_.at(last_applied_ - 1); // Convert to 0-based index
        std::cout << "Applied command at index " << last_applied_ << std::endl;
    }
}

void RaftNode::electionLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        if (state_ == NodeState::LEADER) {
            continue;
        }
        
        if (isElectionTimeoutExpired()) {
            startElection();
        }
    }
}

void RaftNode::heartbeatLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
        
        if (state_ == NodeState::LEADER) {
            sendHeartbeats();
            updateCommitIndex();
        }
    }
}

void RaftNode::applyLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        applyCommittedEntries();
    }
}

void RaftNode::resetElectionTimeout() {
    last_heartbeat_ = std::chrono::steady_clock::now();
    election_timeout_reset_ = true;
}

bool RaftNode::isElectionTimeoutExpired() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat_);
    return elapsed.count() > getElectionTimeout();
}

void RaftNode::initializePeers(const std::vector<std::string>& peer_addresses) {
    for (size_t i = 0; i < peer_addresses.size(); ++i) {
        std::string peer_id = "node_" + std::to_string(i);
        PeerInfo peer_info;
        peer_info.id = peer_id;
        peer_info.address = peer_addresses[i];
        peer_info.stub = nullptr; // Will be initialized with actual gRPC stub later
        
        peers_[peer_id] = std::move(peer_info);
    }
}

int64_t RaftNode::getLastLogIndex() const {
    return log_.getLastLogIndex();
}

int64_t RaftNode::getLastLogTerm() const {
    return log_.getLastLogTerm();
}

bool RaftNode::isLogUpToDate(int64_t last_log_index, int64_t last_log_term) const {
    int64_t our_last_term = getLastLogTerm();
    int64_t our_last_index = getLastLogIndex();
    
    // Log is up-to-date if:
    // 1. Last term is greater, OR
    // 2. Last term is equal and last index is greater or equal
    return (last_log_term > our_last_term) ||
           (last_log_term == our_last_term && last_log_index >= our_last_index);
}

} // namespace raft 