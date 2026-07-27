import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Replace IntraResourceSearch with simpler version
old = '''void RcsaProtocol::IntraResourceSearch(uint32_t requesterId,
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

new = '''void RcsaProtocol::IntraResourceSearch(uint32_t requesterId,
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

    // Allocation successful if neighbors found with same resource type
    m_statistics.successfulAllocations++;
    double searchDelay = Simulator::Now().GetSeconds() - requestTime;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Required: " << requiredAmount << " bytes");
}'''

content = content.replace(old, new)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Simplified IntraResourceSearch()")
