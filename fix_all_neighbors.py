import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

old_find = '''void RcsaProtocol::FindNeighbors() {
    RcsaVehicle* vehicle = GetVehicle(m_vehicleId);
    if (!vehicle) return;

    // Update own position from mobility model
    Ptr<Node> node = GetNode();
    if (node) {
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (mobility) {
            Vector pos = mobility->GetPosition();
            vehicle->SetPosition(pos);
        }
    }

    // Simple neighbor discovery: if resource type matches and distance < range, it's a neighbor
    for (auto& pair2 : m_vehicles) {
        uint32_t neighborId = pair2.first;
        if (neighborId == m_vehicleId) continue;

        RcsaVehicle* neighbor = pair2.second;
        if (!neighbor) continue;
        
        // Both vehicles should have same resource type to be neighbors in RCSA
        if (neighbor->GetResourceType() != vehicle->GetResourceType()) {
            continue;
        }

        double dx = vehicle->GetPosition().x - neighbor->GetPosition().x;
        double dy = vehicle->GetPosition().y - neighbor->GetPosition().y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= m_communicationRange) {
            double connTime = 5.0; // Simple fixed connection time
            vehicle->UpdateNeighbor(neighbor, connTime);
        }
    }
}'''

new_find = '''void RcsaProtocol::FindNeighbors() {
    RcsaVehicle* vehicle = GetVehicle(m_vehicleId);
    if (!vehicle) return;

    // Update own position from mobility model
    Ptr<Node> node = GetNode();
    if (!node) return;
    
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    if (mobility) {
        Vector pos = mobility->GetPosition();
        vehicle->SetPosition(pos);
    }

    // Find neighbors by checking all nodes in the network
    uint32_t numNodes = NodeList::GetNNodes();
    for (uint32_t i = 0; i < numNodes; i++) {
        Ptr<Node> otherNode = NodeList::GetNode(i);
        if (!otherNode || otherNode == node) continue;

        uint32_t neighborId = otherNode->GetId();
        
        // Get neighbor's position
        Ptr<MobilityModel> neighborMobility = otherNode->GetObject<MobilityModel>();
        if (!neighborMobility) continue;
        
        Vector neighborPos = neighborMobility->GetPosition();
        
        // Calculate distance
        double dx = vehicle->GetPosition().x - neighborPos.x;
        double dy = vehicle->GetPosition().y - neighborPos.y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= m_communicationRange) {
            // Found a neighbor within range
            RcsaVehicle* neighbor = GetVehicle(neighborId);
            
            // If not in our map, create it
            if (!neighbor) {
                neighbor = new RcsaVehicle(neighborId, i % 3, 300*1024*1024); // Assume 300MB capacity, type based on ID
                m_vehicles[neighborId] = neighbor;
            }
            
            neighbor->SetPosition(neighborPos);
            
            // Check if same resource type
            if (neighbor->GetResourceType() == vehicle->GetResourceType()) {
                double connTime = 5.0;
                vehicle->UpdateNeighbor(neighbor, connTime);
                NS_LOG_DEBUG("Vehicle " << m_vehicleId << " found neighbor " << neighborId);
            }
        }
    }
}'''

content = content.replace(old_find, new_find)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Fixed FindNeighbors() to use NodeList")
