import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Fix the search delay calculation
old = '''    // Allocation successful if neighbors found with same resource type
    m_statistics.successfulAllocations++;
    double searchDelay = Simulator::Now().GetSeconds() - requestTime;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Required: " << requiredAmount << " bytes");'''

new = '''    // Allocation successful if neighbors found with same resource type
    m_statistics.successfulAllocations++;
    double searchDelay = Simulator::Now().GetSeconds() - requestTime;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Delay: " << searchDelay << "s. Required: " << requiredAmount << " bytes");'''

content = content.replace(old, new)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Fixed search delay logging")
