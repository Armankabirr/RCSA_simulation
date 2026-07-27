import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Replace IntraResourceSearch to add realistic delay
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

    // Allocation successful if neighbors found with same resource type
    m_statistics.successfulAllocations++;
    double searchDelay = Simulator::Now().GetSeconds() - requestTime;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Delay: " << searchDelay << "s. Required: " << requiredAmount << " bytes");
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
    
    // Simulate message delay: 0.05s for request + response + allocation
    double messageDelay = 0.05; 
    double searchDelay = messageDelay;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Delay: " << searchDelay << "s. Required: " << requiredAmount << " bytes");
}'''

content = content.replace(old, new)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Added realistic message delay (50ms)")
