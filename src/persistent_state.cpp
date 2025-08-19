#include "persistent_state.h"
#include <iostream>
#include <sstream>

namespace raft {

PersistentState::PersistentState(const std::string& state_file) 
    : state_file_(state_file) {
    load();
}

PersistentState::~PersistentState() {
    save();
}

int64_t PersistentState::getCurrentTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_term_;
}

void PersistentState::setCurrentTerm(int64_t term) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_term_ = term;
    save();
}

std::string PersistentState::getVotedFor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return voted_for_;
}

void PersistentState::setVotedFor(const std::string& candidate_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    voted_for_ = candidate_id;
    save();
}

bool PersistentState::hasVotedFor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !voted_for_.empty();
}

void PersistentState::clearVote() {
    std::lock_guard<std::mutex> lock(mutex_);
    voted_for_.clear();
    save();
}

void PersistentState::updateTermAndVote(int64_t term, const std::string& voted_for) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_term_ = term;
    voted_for_ = voted_for;
    save();
}

bool PersistentState::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(state_file_, std::ios::binary);
    if (!file.is_open()) {
        // File doesn't exist yet, use defaults
        current_term_ = 0;
        voted_for_.clear();
        return true;
    }
    
    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    file.close();
    
    return deserialize(data);
}

bool PersistentState::save() {
    // Note: mutex should already be held by caller
    
    std::ofstream file(state_file_, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to open state file for writing: " << state_file_ << std::endl;
        return false;
    }
    
    std::string data = serialize();
    file.write(data.c_str(), data.size());
    file.close();
    
    return file.good();
}

std::string PersistentState::serialize() const {
    std::ostringstream oss;
    
    // Write current term
    oss.write(reinterpret_cast<const char*>(&current_term_), sizeof(current_term_));
    
    // Write voted_for length and data
    size_t voted_for_len = voted_for_.size();
    oss.write(reinterpret_cast<const char*>(&voted_for_len), sizeof(voted_for_len));
    
    if (voted_for_len > 0) {
        oss.write(voted_for_.c_str(), voted_for_len);
    }
    
    return oss.str();
}

bool PersistentState::deserialize(const std::string& data) {
    std::istringstream iss(data);
    
    // Read current term
    if (!iss.read(reinterpret_cast<char*>(&current_term_), sizeof(current_term_))) {
        return false;
    }
    
    // Read voted_for length
    size_t voted_for_len;
    if (!iss.read(reinterpret_cast<char*>(&voted_for_len), sizeof(voted_for_len))) {
        return false;
    }
    
    // Read voted_for data
    if (voted_for_len > 0) {
        voted_for_.resize(voted_for_len);
        if (!iss.read(&voted_for_[0], voted_for_len)) {
            return false;
        }
    } else {
        voted_for_.clear();
    }
    
    return true;
}

} // namespace raft 