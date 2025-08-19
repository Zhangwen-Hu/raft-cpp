#include "raft_server.h"
#include "raft_node.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace raft;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <node_id> <listen_address> [peer_address1] [peer_address2] ...\n";
    std::cout << "Example: " << program_name << " node1 localhost:50051 localhost:50052 localhost:50053\n";
    std::cout << "\n";
    std::cout << "This will start a Raft node with the given ID listening on the specified address.\n";
    std::cout << "The peer addresses should be the addresses of other Raft nodes in the cluster.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string node_id = argv[1];
    std::string listen_address = argv[2];
    
    std::vector<std::string> peer_addresses;
    for (int i = 3; i < argc; ++i) {
        peer_addresses.push_back(argv[i]);
    }
    
    std::cout << "Starting Raft node:\n";
    std::cout << "  Node ID: " << node_id << "\n";
    std::cout << "  Listen Address: " << listen_address << "\n";
    std::cout << "  Peers: ";
    for (const auto& peer : peer_addresses) {
        std::cout << peer << " ";
    }
    std::cout << "\n\n";
    
    try {
        // Create Raft node
        auto raft_node = std::make_unique<RaftNode>(node_id, listen_address, peer_addresses);
        
        // Create and start server
        RaftServer server(std::move(raft_node), listen_address);
        server.start();
        
        std::cout << "Raft server is running. Press Ctrl+C to stop.\n\n";
        
        // Demonstration: Submit some commands if we become leader  
        std::thread demo_thread([&server]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            auto* node = server.getRaftNode();
            for (int i = 0; i < 10; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                if (node->isLeader()) {
                    std::string command = "command_" + std::to_string(i);
                    std::vector<uint8_t> command_bytes(command.begin(), command.end());
                    
                    if (node->submitCommand(command_bytes)) {
                        std::cout << "Submitted command: " << command << std::endl;
                    }
                } else {
                    std::cout << "Node state: ";
                    switch (node->getState()) {
                        case NodeState::FOLLOWER:
                            std::cout << "FOLLOWER";
                            break;
                        case NodeState::CANDIDATE:
                            std::cout << "CANDIDATE";
                            break;
                        case NodeState::LEADER:
                            std::cout << "LEADER";
                            break;
                    }
                    std::cout << ", Term: " << node->getCurrentTerm();
                    if (!node->getLeaderId().empty()) {
                        std::cout << ", Leader: " << node->getLeaderId();
                    }
                    std::cout << std::endl;
                }
            }
        });
        
        // Keep the server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        demo_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 