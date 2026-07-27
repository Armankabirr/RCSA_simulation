#ifndef RCSA_CLUSTER_H
#define RCSA_CLUSTER_H

#include "rcsa-vehicle.h"
#include <set>

namespace ns3 {

class RcsaCluster {
public:
    RcsaCluster(uint32_t clusterId, uint32_t resourceType);
    ~RcsaCluster();

    // Getters
    uint32_t GetClusterId() const { return m_clusterId; }
    uint32_t GetResourceType() const { return m_resourceType; }
    uint32_t GetClusterHeadId() const { return m_clusterHeadId; }
    RcsaVehicle* GetClusterHead() const { return m_clusterHead; }
    const std::set<uint32_t>& GetMembers() const { return m_memberIds; }
    uint64_t GetTotalAvailableResource() const;

    // Cluster operations
    void AddMember(RcsaVehicle* vehicle);
    void RemoveMember(uint32_t vehicleId);
    bool HasMember(uint32_t vehicleId) const;
    RcsaVehicle* GetMember(uint32_t vehicleId) const;

    // Cluster head operations
    void ElectClusterHead(double timeStep, double communicationRange);
    void ReplaceClusterHead(double timeStep, double communicationRange);
    bool IsClusterHeadValid() const;

    // Resource allocation
    struct AllocationCandidate {
        std::vector<uint32_t> selectedMembers;
        uint64_t totalResourceProvided;
        double averageConnectionTime;
    };

    AllocationCandidate SelectResourceMembers(uint64_t requiredResource, 
                                             uint32_t requesterVehicleId);

    // Status
    bool IsEmpty() const { return m_memberIds.empty(); }
    uint32_t GetMemberCount() const { return m_memberIds.size(); }

    // Resource information
    void UpdateMemberResource(uint32_t vehicleId, uint64_t availableAmount);

private:
    uint32_t m_clusterId;
    uint32_t m_resourceType;
    uint32_t m_clusterHeadId;
    RcsaVehicle* m_clusterHead;
    std::set<uint32_t> m_memberIds;
    std::map<uint32_t, RcsaVehicle*> m_vehicleMap;

    // Helper functions
    RcsaVehicle* FindBestClusterHead(double timeStep, double communicationRange);
    std::vector<AllocationCandidate> GenerateAllocationCombinations(
        uint64_t requiredResource, uint32_t requesterVehicleId);
};

} // namespace ns3

#endif // RCSA_CLUSTER_H
