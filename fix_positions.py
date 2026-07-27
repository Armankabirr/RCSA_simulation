import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Replace FindNeighbors to use actual positions from mobility model
old_find = '''void RcsaProtocol::FindNeighbors() {
    // For local vehicle, find neighbors within communication range
    RcsaVehicle* vehicle = GetVehicle(m_vehicleId);
    if (!vehicle) return;

    for (auto& pair2 : m_vehicles) {
        uint32_t neighborId = pair2.first;
        if (neighborId == m_vehicleId) continue;

        RcsaVehicle* neighbor = pair2.second;
        if (!neighbor) continue;
        
        double dx = vehicle->GetPosition().x - neighbor->GetPosition().x;
        double dy = vehicle->GetPosition().y - neighbor->GetPosition().y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= m_communicationRange) {
            // Calculate connection probability
            double connProb = vehicle->CalculateConnectionProbability(
                neighbor, m_beaconInterval, m_communicationRange);

            if (connProb > 0.0) {
                // Connection time is proportional to probability * timeout
                double connTime = connProb * m_clusterHeadTimeout;
                vehicle->UpdateNeighbor(neighbor, connTime);
                NS_LOG_DEBUG("Vehicle " << m_vehicleId << " found neighbor " << neighborId);
            }
        } else {
            // Remove if out of range
            vehicle->RemoveNeighbor(neighborId);
        }
    }
}'''

new_find = '''void RcsaProtocol::FindNeighbors() {
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

content = content.replace(old_find, new_find)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Fixed FindNeighbors() to use real positions")
