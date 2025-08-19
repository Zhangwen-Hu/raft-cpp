#include "log_entry.h"
#include <sstream>
#include <stdexcept>

namespace raft {

LogEntry::LogEntry(int64_t term, int64_t index, const std::vector<uint8_t>& command)
    : term_(term), index_(index), command_(command) {
}

std::string LogEntry::serialize() const {
    std::ostringstream oss;
    oss.write(reinterpret_cast<const char*>(&term_), sizeof(term_));
    oss.write(reinterpret_cast<const char*>(&index_), sizeof(index_));
    
    size_t command_size = command_.size();
    oss.write(reinterpret_cast<const char*>(&command_size), sizeof(command_size));
    
    if (!command_.empty()) {
        oss.write(reinterpret_cast<const char*>(command_.data()), command_size);
    }
    
    return oss.str();
}

LogEntry LogEntry::deserialize(const std::string& data) {
    std::istringstream iss(data);
    
    int64_t term, index;
    iss.read(reinterpret_cast<char*>(&term), sizeof(term));
    iss.read(reinterpret_cast<char*>(&index), sizeof(index));
    
    size_t command_size;
    iss.read(reinterpret_cast<char*>(&command_size), sizeof(command_size));
    
    std::vector<uint8_t> command(command_size);
    if (command_size > 0) {
        iss.read(reinterpret_cast<char*>(command.data()), command_size);
    }
    
    return LogEntry(term, index, command);
}

// Log implementation

void Log::append(const LogEntry& entry) {
    entries_.push_back(entry);
}

void Log::append(const std::vector<LogEntry>& entries) {
    entries_.insert(entries_.end(), entries.begin(), entries.end());
}

bool Log::removeEntriesFrom(int64_t index) {
    if (index <= 0 || index > getLastLogIndex()) {
        return false;
    }
    
    size_t vector_index = toVectorIndex(index);
    entries_.erase(entries_.begin() + vector_index, entries_.end());
    return true;
}

int64_t Log::getLastLogIndex() const {
    if (entries_.empty()) {
        return 0;
    }
    return toRaftIndex(entries_.size() - 1);
}

int64_t Log::getLastLogTerm() const {
    if (entries_.empty()) {
        return 0;
    }
    return entries_.back().getTerm();
}

int64_t Log::getTermAt(int64_t index) const {
    if (index <= 0 || index > getLastLogIndex()) {
        return 0;
    }
    
    size_t vector_index = toVectorIndex(index);
    return entries_[vector_index].getTerm();
}

std::vector<LogEntry> Log::getEntries(int64_t start_index, int64_t end_index) const {
    if (start_index <= 0 || start_index > getLastLogIndex()) {
        return {};
    }
    
    if (end_index == -1) {
        end_index = getLastLogIndex() + 1;
    }
    
    size_t start_vector_index = toVectorIndex(start_index);
    size_t end_vector_index = (end_index > getLastLogIndex()) ? 
        entries_.size() : toVectorIndex(end_index);
    
    if (start_vector_index >= end_vector_index) {
        return {};
    }
    
    return std::vector<LogEntry>(
        entries_.begin() + start_vector_index,
        entries_.begin() + end_vector_index
    );
}

bool Log::hasEntryAt(int64_t index, int64_t term) const {
    if (index <= 0 || index > getLastLogIndex()) {
        return false;
    }
    
    return getTermAt(index) == term;
}

void Log::clear() {
    entries_.clear();
}

} // namespace raft 