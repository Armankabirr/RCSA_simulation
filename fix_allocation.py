import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Replace IntraResourceSearch
old_search = '''void RcsaProtocol::IntraResourceSearch(uint32_t requesterId,
                                       uint32_t resourceType,
                                       uint64_t requiredAmount,
                                       double duration) {
    NS_LOG_FUNCTION(this << requesterId << resourceType << requiredAmount << duration);

    m_statistics.requestsSent++;
    double requestTime = Simulator::Now().GetSeconds();
    m_requestStartTimes[requesterId] = requestTime;

    RcsaVehicle* requester = GetVehicle(requesterId);
    if (!requester) {
        m_statistics.failedAllocations++;
        return;
    }

    // Check if requester has neighbor with same resource type (Algorithm 3)
    std::vector<uint32_t> sameTypeNeighbors =
        requester->GetNeighborsWithResourceType(resourceType);

    if (sameTypeNeighbors.empty()) {
        NS_LOG_INFO("No neighbors with resource type " << resourceType
                                                        << " for vehicle "
                                                        << requesterId);
        m_statistics.failedAllocations++;
        return;
    }

    // Get the cluster
    RcsaCluster* cluster = GetClusterByType(resourceType);
    if (!cluster || !cluster->IsClusterHeadValid()) {
        NS_LOG_INFO("No valid cluster for resource type " << resourceType);
        m_statistics.failedAllocations++;
        return;
    }

    // Request to cluster head via RCM neighbor
    RcsaCluster::AllocationCandidate allocation =
        cluster->SelectResourceMembers(requiredAmount, requesterId);

    if (allocation.totalResourceProvided >= requiredAmount) {
        // Allocation successful
        m_statistics.successfulAllocations++;

        // Allocate resources
        for (uint32_t rcmId : allocation.selectedMembers) {
            RcsaVehicle* rcm = GetVehicle(rcmId);
            if (rcm) {
                rcm->AllocateResource(
                    requiredAmount / allocation.selectedMembers.size());
            }
        }

        // Record search delay
        double searchDelay = Simulator::Now().GetSeconds() - requestTime;
        m_statistics.totalSearchDelay += searchDelay;

        NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                    << requesterId << ". Delay: " << searchDelay << "s");
    } else {
        NS_LOG_INFO("Insufficient resources in cluster " << cluster->GetClusterId());
        m_statistics.failedAllocations++;
    }
}'''

new_search = '''void RcsaProtocol::IntraResourceSearch(uint32_t requesterId,
                                       uint32_t resourceType,
                                       uint64_t requiredAmount,
                                       double duration) {
    NS_LOG_FUNCTION(this << requesterId << resourceType << requiredAmount << duration);

    m_statistics.requestsSent++;
    double requestTime = Simulator::Now().GetSeconds();
    m_requestStartTimes[requesterId] = requestTime;

    RcsaVehicle* requester = GetVehicle(requesterId);
    if (!requester) {
        m_statistics.failedAllocations++;
        return;
    }

    // Check if requester has neighbor with same resource type
    std::vector<uint32_t> sameTypeNeighbors =
        requester->GetNeighborsWithResourceType(resourceType);

    if (sameTypeNeighbors.empty()) {
        NS_LOG_DEBUG("No neighbors with resource type " << resourceType
                    << " for vehicle " << requesterId);
        m_statistics.failedAllocations++;
        return;
    }

    // Try to allocate from neighbors
    uint64_t totalAllocated = 0;
    std::vector<uint32_t> allocators;

    for (uint32_t neighborId : sameTypeNeighbors) {
        RcsaVehicle* neighbor = GetVehicle(neighborId);
        if (!neighbor) continue;

        uint64_t available = neighbor->GetAvailableResource();
        if (available > 0) {
            uint64_t toAllocate = std::min(available, requiredAmount - totalAllocated);
            neighbor->AllocateResource(toAllocate);
            totalAllocated += toAllocate;
            allocators.push_back(neighborId);

            if (totalAllocated >= requiredAmount) break;
        }
    }

    if (totalAllocated >= requiredAmount) {
        m_statistics.successfulAllocations++;
        double searchDelay = Simulator::Now().GetSeconds() - requestTime;
        m_statistics.totalSearchDelay += searchDelay;

        NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                    << requesterId << " from " << allocators.size() 
                    << " neighbors. Allocated: " << totalAllocated << " bytes");
    } else {
        NS_LOG_DEBUG("Insufficient resources for vehicle " << requesterId);
        m_statistics.failedAllocations++;
    }
}'''

content = content.replace(old_search, new_search)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Fixed IntraResourceSearch()")
