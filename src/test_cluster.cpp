#include "raft_server.h"
#include "raft_node.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>

using namespace raft;

int main() {
    std::cout << "Starting Raft test cluster with 3 nodes...\n\n";
    
    // Configuration for 3-node cluster
    std::vector<std::string> addresses = {
        "localhost:50051",
        "localhost:50052", 
        "localhost:50053"
    };
    
    std::vector<std::string> node_ids = {
        "node1",
        "node2", 
        "node3"
    };
    
    std::vector<std::unique_ptr<RaftServer>> servers;
    std::vector<std::thread> server_threads;
    
    try {
        // Create and start all nodes
        for (size_t i = 0; i < addresses.size(); ++i) {
            std::vector<std::string> peer_addresses;
            
            // Add all other nodes as peers
            for (size_t j = 0; j < addresses.size(); ++j) {
                if (i != j) {
                    peer_addresses.push_back(addresses[j]);
                }
            }
            
            std::cout << "Creating " << node_ids[i] << " listening on " << addresses[i] << std::endl;
            std::cout << "  Peers: ";
            for (const auto& peer : peer_addresses) {
                std::cout << peer << " ";
            }
            std::cout << "\n";
            
            // Create Raft node
            auto raft_node = std::make_unique<RaftNode>(node_ids[i], addresses[i], peer_addresses);
            
            // Create server
            auto server = std::make_unique<RaftServer>(std::move(raft_node), addresses[i]);
            
            servers.push_back(std::move(server));
        }
        
        std::cout << "\nStarting all servers...\n";
        
        // Start all servers
        for (auto& server : servers) {
            server->start();
        }
        
        std::cout << "\nAll servers started. Monitoring cluster state...\n\n";
        
        // Monitor the cluster for 30 seconds
        for (int iteration = 0; iteration < 30; ++iteration) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            std::cout << "=== Iteration " << (iteration + 1) << " ===\n";
            
            std::string leader_id;
            int leader_count = 0;
            
            for (size_t i = 0; i < servers.size(); ++i) {
                auto* node = servers[i]->getRaftNode();
                
                std::cout << node_ids[i] << ": ";
                
                switch (node->getState()) {
                    case NodeState::FOLLOWER:
                        std::cout << "FOLLOWER";
                        break;
                    case NodeState::CANDIDATE:
                        std::cout << "CANDIDATE";
                        break;
                    case NodeState::LEADER:
                        std::cout << "LEADER";
                        leader_id = node_ids[i];
                        leader_count++;
                        break;
                }
                
                std::cout << " (term " << node->getCurrentTerm() << ")";
                
                if (!node->getLeaderId().empty()) {
                    std::cout << " [leader: " << node->getLeaderId() << "]";
                }
                
                std::cout << std::endl;
            }
            
            if (leader_count == 1) {
                std::cout << "Cluster has elected leader: " << leader_id << std::endl;
                
                // Try to submit a command through the leader
                for (size_t i = 0; i < servers.size(); ++i) {
                    auto* node = servers[i]->getRaftNode();
                    if (node->isLeader()) {
                        std::string command = "test_command_" + std::to_string(iteration);
                        std::vector<uint8_t> command_bytes(command.begin(), command.end());
                        
                        if (node->submitCommand(command_bytes)) {
                            std::cout << "Successfully submitted: " << command << std::endl;
                        }
                        break;
                    }
                }
            } else if (leader_count > 1) {
                std::cout << "ERROR: Multiple leaders detected!" << std::endl;
            } else {
                std::cout << "No leader elected yet." << std::endl;
            }
            
            std::cout << std::endl;
        }
        
        std::cout << "Test completed. Stopping all servers...\n";
        
        // Stop all servers
        for (auto& server : servers) {
            server->stop();
        }
        
        std::cout << "All servers stopped.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 