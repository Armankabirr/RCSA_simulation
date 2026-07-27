import re

with open('rcsa-protocol.cc', 'r') as f:
    content = f.read()

# Replace FindNeighbors function
old_find = '''void RcsaProtocol::FindNeighbors() {
    // For each vehicle, find neighbors within communication range
    for (auto& pair1 : m_vehicles) {
        RcsaVehicle* vehicle = pair1.second;

        for (auto& pair2 : m_vehicles) {
            if (pair1.first == pair2.first) continue;

            RcsaVehicle* neighbor = pair2.second;
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
                }
            } else {
                // Remove if out of range
                vehicle->RemoveNeighbor(neighbor->GetId());
            }
        }
    }
}'''

new_find = '''void RcsaProtocol::FindNeighbors() {
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

content = content.replace(old_find, new_find)

with open('rcsa-protocol.cc', 'w') as f:
    f.write(content)

print("✅ Fixed FindNeighbors()")
