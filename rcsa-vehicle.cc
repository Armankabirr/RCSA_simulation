#include "rcsa-vehicle.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace ns3 {

RcsaVehicle::RcsaVehicle(uint32_t id, uint32_t resourceType, uint64_t capacity)
    : m_id(id),
      m_resourceType(resourceType),
      m_isClusterHead(false),
      m_clusterId(0xFFFFFFFF),
      m_lastUpdateTime(0.0) {
    m_resource.resourceType = resourceType;
    m_resource.availableAmount = capacity;
    m_resource.totalCapacity = capacity;
    m_position = Vector(0, 0, 0);
    m_velocity = Vector(0, 0, 0);
}

RcsaVehicle::~RcsaVehicle() {
    m_neighbors.clear();
}

void RcsaVehicle::UpdateNeighbor(const RcsaVehicle* neighbor, double connectionTime) {
    if (!neighbor || neighbor->GetId() == m_id) {
        return;
    }

    NeighborInfo info;
    info.vehicleId = neighbor->GetId();
    info.resourceType = neighbor->GetResourceType();
    info.resourceAmount = neighbor->GetAvailableResource();
    info.position = neighbor->GetPosition();
    info.velocity = neighbor->GetVelocity();
    info.connectionTime = connectionTime;

    m_neighbors[neighbor->GetId()] = info;
}

void RcsaVehicle::RemoveNeighbor(uint32_t vehicleId) {
    m_neighbors.erase(vehicleId);
}

NeighborInfo* RcsaVehicle::GetNeighbor(uint32_t vehicleId) {
    auto it = m_neighbors.find(vehicleId);
    if (it != m_neighbors.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<uint32_t> RcsaVehicle::GetNeighborsWithResourceType(uint32_t resourceType) {
    std::vector<uint32_t> result;
    for (auto& pair : m_neighbors) {
        if (pair.second.resourceType == resourceType) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void RcsaVehicle::AllocateResource(uint64_t amount) {
    if (m_resource.availableAmount >= amount) {
        m_resource.availableAmount -= amount;
    }
}

void RcsaVehicle::DeallocateResource(uint64_t amount) {
    m_resource.availableAmount += amount;
    if (m_resource.availableAmount > m_resource.totalCapacity) {
        m_resource.availableAmount = m_resource.totalCapacity;
    }
}

bool RcsaVehicle::HasSufficientResource(uint64_t required) const {
    return m_resource.availableAmount >= required;
}

// Connection Probability Calculation (from paper Eq. 1-15)
double RcsaVehicle::CalculateConnectionProbability(const RcsaVehicle* other, 
                                                    double timeStep,
                                                    double communicationRange) const {
    if (!other || other->GetId() == m_id) {
        return 0.0;
    }

    // Calculate distance between two vehicles
    double dx = m_position.x - other->GetPosition().x;
    double dy = m_position.y - other->GetPosition().y;
    double distance = std::sqrt(dx * dx + dy * dy);

    // Simple connection probability based on distance
    // If distance > communication range, probability = 0
    if (distance > communicationRange) {
        return 0.0;
    }

    // Probability decreases with distance
    double probability = 1.0 - (distance / communicationRange);

    // Factor in velocity differences (mobility)
    double dvx = m_velocity.x - other->GetVelocity().x;
    double dvy = m_velocity.y - other->GetVelocity().y;
    double relativeVelocity = std::sqrt(dvx * dvx + dvy * dvy);

    // Vehicles with similar velocity have higher connection probability
    double maxVelocity = 25.0; // 90 km/h in m/s
    double velocityFactor = 1.0 - (relativeVelocity / (2.0 * maxVelocity));
    velocityFactor = std::max(0.0, velocityFactor);

    probability *= (0.7 + 0.3 * velocityFactor);

    return probability;
}

// Calculate average connection time with all neighbors
double RcsaVehicle::CalculateAverageConnectionTime(double timeStep, 
                                                   double communicationRange) {
    if (m_neighbors.empty()) {
        return 0.0;
    }

    double totalConnectionTime = 0.0;
    for (auto& pair : m_neighbors) {
        totalConnectionTime += pair.second.connectionTime;
    }

    return totalConnectionTime / m_neighbors.size();
}

} // namespace ns3
