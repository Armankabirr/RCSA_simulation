#ifndef RCSA_VEHICLE_H
#define RCSA_VEHICLE_H

#include "ns3/node.h"
#include "ns3/vector.h"
#include <map>
#include <vector>
#include <cmath>

namespace ns3 {

struct ResourceInfo {
    uint32_t resourceType;
    uint64_t availableAmount;
    uint64_t totalCapacity;
};

struct NeighborInfo {
    uint32_t vehicleId;
    uint32_t resourceType;
    uint64_t resourceAmount;
    Vector position;
    Vector velocity;
    double connectionTime;
};

class RcsaVehicle {
public:
    RcsaVehicle(uint32_t id, uint32_t resourceType, uint64_t capacity);
    ~RcsaVehicle();

    // Getters
    uint32_t GetId() const { return m_id; }
    uint32_t GetResourceType() const { return m_resourceType; }
    uint64_t GetAvailableResource() const { return m_resource.availableAmount; }
    uint64_t GetTotalCapacity() const { return m_resource.totalCapacity; }
    Vector GetPosition() const { return m_position; }
    Vector GetVelocity() const { return m_velocity; }
    bool IsClusterHead() const { return m_isClusterHead; }
    uint32_t GetClusterId() const { return m_clusterId; }

    // Setters
    void SetPosition(Vector pos) { m_position = pos; }
    void SetVelocity(Vector vel) { m_velocity = vel; }
    void SetAsClusterHead(uint32_t clusterId) { 
        m_isClusterHead = true; 
        m_clusterId = clusterId;
    }
    void UnsetClusterHead() { 
        m_isClusterHead = false; 
        m_clusterId = 0xFFFFFFFF;
    }

    // Neighbor management
    void UpdateNeighbor(const RcsaVehicle* neighbor, double connectionTime);
    void RemoveNeighbor(uint32_t vehicleId);
    const std::map<uint32_t, NeighborInfo>& GetNeighbors() const { return m_neighbors; }
    NeighborInfo* GetNeighbor(uint32_t vehicleId);
    std::vector<uint32_t> GetNeighborsWithResourceType(uint32_t resourceType);

    // Resource management
    void AllocateResource(uint64_t amount);
    void DeallocateResource(uint64_t amount);
    bool HasSufficientResource(uint64_t required) const;

    // Connection probability calculation
    double CalculateConnectionProbability(const RcsaVehicle* other, double timeStep, 
                                         double communicationRange) const;
    double CalculateAverageConnectionTime(double timeStep, double communicationRange);

private:
    uint32_t m_id;
    uint32_t m_resourceType;
    ResourceInfo m_resource;
    Vector m_position;
    Vector m_velocity;
    bool m_isClusterHead;
    uint32_t m_clusterId;
    std::map<uint32_t, NeighborInfo> m_neighbors;

    // For connection probability calculation
    double m_lastUpdateTime;
};

} // namespace ns3

#endif // RCSA_VEHICLE_H
