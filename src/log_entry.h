#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace raft {

class LogEntry {
public:
    LogEntry() = default;
    LogEntry(int64_t term, int64_t index, const std::vector<uint8_t>& command);
    
    int64_t getTerm() const { return term_; }
    int64_t getIndex() const { return index_; }
    const std::vector<uint8_t>& getCommand() const { return command_; }
    
    void setTerm(int64_t term) { term_ = term; }
    void setIndex(int64_t index) { index_ = index; }
    void setCommand(const std::vector<uint8_t>& command) { command_ = command; }
    
    // Serialization helpers
    std::string serialize() const;
    static LogEntry deserialize(const std::string& data);

private:
    int64_t term_ = 0;
    int64_t index_ = 0;
    std::vector<uint8_t> command_;
};

class Log {
public:
    Log() = default;
    
    // Log operations
    void append(const LogEntry& entry);
    void append(const std::vector<LogEntry>& entries);
    bool removeEntriesFrom(int64_t index);
    
    // Getters
    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }
    const LogEntry& at(size_t index) const { return entries_.at(index); }
    const LogEntry& back() const { return entries_.back(); }
    
    // Log indices (1-based as per Raft spec)
    int64_t getLastLogIndex() const;
    int64_t getLastLogTerm() const;
    int64_t getTermAt(int64_t index) const;
    
    // Get entries in range [start_index, end_index)
    std::vector<LogEntry> getEntries(int64_t start_index, int64_t end_index = -1) const;
    
    // Check if log contains entry at index with given term
    bool hasEntryAt(int64_t index, int64_t term) const;
    
    // Clear all entries
    void clear();

private:
    std::vector<LogEntry> entries_;
    
    // Convert between 1-based Raft indices and 0-based vector indices
    size_t toVectorIndex(int64_t raft_index) const { return static_cast<size_t>(raft_index - 1); }
    int64_t toRaftIndex(size_t vector_index) const { return static_cast<int64_t>(vector_index + 1); }
};

} // namespace raft 