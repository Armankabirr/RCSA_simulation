#include "rcsa-cluster.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ns3 {

RcsaCluster::RcsaCluster(uint32_t clusterId, uint32_t resourceType)
    : m_clusterId(clusterId),
      m_resourceType(resourceType),
      m_clusterHeadId(0xFFFFFFFF),
      m_clusterHead(nullptr) {
}

RcsaCluster::~RcsaCluster() {
    m_memberIds.clear();
    m_vehicleMap.clear();
    m_clusterHead = nullptr;
}

void RcsaCluster::AddMember(RcsaVehicle* vehicle) {
    if (!vehicle || vehicle->GetResourceType() != m_resourceType) {
        return;
    }

    uint32_t vehicleId = vehicle->GetId();
    if (m_memberIds.find(vehicleId) == m_memberIds.end()) {
        m_memberIds.insert(vehicleId);
        m_vehicleMap[vehicleId] = vehicle;
    }
}

void RcsaCluster::RemoveMember(uint32_t vehicleId) {
    m_memberIds.erase(vehicleId);
    m_vehicleMap.erase(vehicleId);

    // If cluster head is removed, elect new one
    if (m_clusterHeadId == vehicleId) {
        m_clusterHead = nullptr;
        m_clusterHeadId = 0xFFFFFFFF;
    }
}

bool RcsaCluster::HasMember(uint32_t vehicleId) const {
    return m_memberIds.find(vehicleId) != m_memberIds.end();
}

RcsaVehicle* RcsaCluster::GetMember(uint32_t vehicleId) const {
    auto it = m_vehicleMap.find(vehicleId);
    if (it != m_vehicleMap.end()) {
        return it->second;
    }
    return nullptr;
}

uint64_t RcsaCluster::GetTotalAvailableResource() const {
    uint64_t total = 0;
    for (uint32_t memberId : m_memberIds) {
        auto it = m_vehicleMap.find(memberId);
        if (it != m_vehicleMap.end()) {
            total += it->second->GetAvailableResource();
        }
    }
    return total;
}

// Algorithm 1.2 from paper: Election of Resource Cluster Headers
RcsaVehicle* RcsaCluster::FindBestClusterHead(double timeStep, 
                                              double communicationRange) {
    if (m_memberIds.empty()) {
        return nullptr;
    }

    RcsaVehicle* bestClusterHead = nullptr;
    double highestScore = -1.0;

    for (uint32_t memberId : m_memberIds) {
        RcsaVehicle* candidate = m_vehicleMap[memberId];
        if (!candidate) continue;

        // Calculate average connection time with all other members
        double totalConnectionTime = 0.0;
        int connectionCount = 0;

        for (uint32_t otherId : m_memberIds) {
            if (otherId == memberId) continue;
            RcsaVehicle* other = m_vehicleMap[otherId];
            if (!other) continue;

            double connProb = candidate->CalculateConnectionProbability(
                other, timeStep, communicationRange);
            if (connProb > 0.0) {
                totalConnectionTime += connProb * 100; // Simulate connection time
                connectionCount++;
            }
        }

        double avgConnectionTime = (connectionCount > 0) 
            ? (totalConnectionTime / connectionCount) 
            : 0.0;

        // Score = average connection time + available resource factor
        double resourceFactor = static_cast<double>(candidate->GetAvailableResource()) 
                              / candidate->GetTotalCapacity();
        double score = avgConnectionTime + (resourceFactor * 50);

        if (score > highestScore) {
            highestScore = score;
            bestClusterHead = candidate;
        }
    }

    return bestClusterHead;
}

void RcsaCluster::ElectClusterHead(double timeStep, double communicationRange) {
    RcsaVehicle* newHead = FindBestClusterHead(timeStep, communicationRange);
    if (newHead) {
        m_clusterHead = newHead;
        m_clusterHeadId = newHead->GetId();
        newHead->SetAsClusterHead(m_clusterId);
    }
}

void RcsaCluster::ReplaceClusterHead(double timeStep, double communicationRange) {
    // If current head is not in cluster anymore, elect new one
    if (m_clusterHead && !HasMember(m_clusterHead->GetId())) {
        ElectClusterHead(timeStep, communicationRange);
    }
}

bool RcsaCluster::IsClusterHeadValid() const {
    return m_clusterHead != nullptr && HasMember(m_clusterHeadId);
}

void RcsaCluster::UpdateMemberResource(uint32_t vehicleId, uint64_t availableAmount) {
    auto it = m_vehicleMap.find(vehicleId);
    if (it != m_vehicleMap.end()) {
        // Update internal tracking if needed
    }
}

// Resource allocation algorithm (based on paper Section 4.2.3)
std::vector<RcsaCluster::AllocationCandidate> 
RcsaCluster::GenerateAllocationCombinations(uint64_t requiredResource,
                                            uint32_t requesterVehicleId) {
    std::vector<AllocationCandidate> candidates;
    std::vector<std::pair<uint32_t, uint64_t>> availableMembers;

    // Collect available members (excluding requester if it's a member)
    for (uint32_t memberId : m_memberIds) {
        if (memberId == requesterVehicleId) continue;
        RcsaVehicle* member = m_vehicleMap[memberId];
        if (member && member->HasSufficientResource(1)) {
            availableMembers.push_back({memberId, member->GetAvailableResource()});
        }
    }

    // Generate combinations greedily (simpler approach than full combination generation)
    // Sort by available resource in descending order
    std::sort(availableMembers.begin(), availableMembers.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Generate minimal combinations
    uint64_t accumulated = 0;
    AllocationCandidate current;
    current.totalResourceProvided = 0;
    current.averageConnectionTime = 0.0;

    for (auto& pair : availableMembers) {
        uint32_t memberId = pair.first;
        uint64_t available = pair.second;
        RcsaVehicle* member = m_vehicleMap[memberId];

        current.selectedMembers.push_back(memberId);
        current.totalResourceProvided += available;
        
        if (current.totalResourceProvided >= requiredResource) {
            candidates.push_back(current);
            break; // Found minimal solution
        }
    }

    return candidates;
}

RcsaCluster::AllocationCandidate 
RcsaCluster::SelectResourceMembers(uint64_t requiredResource, 
                                   uint32_t requesterVehicleId) {
    AllocationCandidate result;
    result.totalResourceProvided = 0;
    result.averageConnectionTime = 0.0;

    auto candidates = GenerateAllocationCombinations(requiredResource, requesterVehicleId);
    
    if (!candidates.empty()) {
        // Select the allocation with minimum members and highest connection time
        result = candidates[0];
        for (auto& cand : candidates) {
            if (cand.selectedMembers.size() < result.selectedMembers.size()) {
                result = cand;
            }
        }
    }

    return result;
}

} // namespace ns3
