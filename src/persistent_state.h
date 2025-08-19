#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <cstdint>

namespace raft {

class PersistentState {
public:
    explicit PersistentState(const std::string& state_file);
    ~PersistentState();
    
    // Persistent state variables (must be updated on stable storage before responding to RPCs)
    int64_t getCurrentTerm() const;
    void setCurrentTerm(int64_t term);
    
    // For voted_for: empty string means no vote, non-empty means voted for that candidate
    std::string getVotedFor() const;
    void setVotedFor(const std::string& candidate_id);
    bool hasVotedFor() const;
    void clearVote();
    
    // Load and save state to disk
    bool load();
    bool save();
    
    // Thread-safe operations
    void updateTermAndVote(int64_t term, const std::string& voted_for);

private:
    mutable std::mutex mutex_;
    std::string state_file_;
    
    // Raft persistent state
    int64_t current_term_ = 0;
    std::string voted_for_; // empty string means no vote cast
    
    // File format helpers
    std::string serialize() const;
    bool deserialize(const std::string& data);
};

} // namespace raft 