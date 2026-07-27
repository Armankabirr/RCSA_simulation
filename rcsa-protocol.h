#ifndef RCSA_PROTOCOL_H
#define RCSA_PROTOCOL_H

#include "rcsa-vehicle.h"
#include "rcsa-cluster.h"
#include "rcsa-messages.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include <map>
#include <set>

namespace ns3 {

class RcsaProtocol : public Application {
public:
    static TypeId GetTypeId();

    RcsaProtocol();
    virtual ~RcsaProtocol();

    // Application lifecycle
    virtual void StartApplication();
    virtual void StopApplication();

    // Configuration
    void SetBeaconInterval(double interval) { m_beaconInterval = interval; }
    void SetCommunicationRange(double range) { m_communicationRange = range; }
    void SetResourceType(uint32_t type) { m_resourceType = type; }
    void SetResourceCapacity(uint64_t capacity) { m_resourceCapacity = capacity; }
    void SetClusterHeadTimeout(double timeout) { m_clusterHeadTimeout = timeout; }

    // Vehicle management
    void RegisterVehicle(uint32_t vehicleId, uint32_t resourceType, uint64_t capacity);
    void UpdateVehiclePosition(uint32_t vehicleId, double x, double y);
    void UpdateVehicleVelocity(uint32_t vehicleId, double vx, double vy);
    RcsaVehicle* GetVehicle(uint32_t vehicleId) const;

    // Cluster operations
    void ConstructResourceClusters();
    void UpdateResourceClusters();
    RcsaCluster* GetClusterByType(uint32_t resourceType) const;
    RcsaCluster* GetClusterById(uint32_t clusterId) const;

    // Resource search and allocation - Algorithm 2 (Intra-resource)
    void IntraResourceSearch(uint32_t requesterId, uint32_t resourceType, 
                           uint64_t requiredAmount, double duration);

    // Resource search and allocation - Algorithm 4 (Inter-resource)
    void InterResourceSearch(uint32_t requesterId, uint32_t resourceType, 
                           uint64_t requiredAmount, double duration);

    // Message handling
    void HandleBeaconMessage(Ptr<Packet> packet, Ipv4Address srcAddr);
    void HandleResourceRequest(Ptr<Packet> packet, Ipv4Address srcAddr);
    void HandleAllocationMessage(Ptr<Packet> packet, Ipv4Address srcAddr);

    // Statistics
    struct Statistics {
        uint32_t beaconsSent;
        uint32_t requestsSent;
        uint32_t allocationsSent;
        double totalSearchDelay;
        uint32_t successfulAllocations;
        uint32_t failedAllocations;
        uint32_t totalPacketsGenerated;
    };

    Statistics GetStatistics() const { return m_statistics; }
    void ResetStatistics();

protected:
    virtual void DoDispose();

private:
    // Core RCSA operations
    void SendBeacon();
    void BroadcastBeaconToNeighbors();
    void ProcessBeaconMessages();
    
    // Resource Cluster-Based Clustering (Section 4.1)
    void PerformResourceTypeClustering();
    void ElectClusterHeads();
    void MaintainClusters();

    // Helper methods
    void FindNeighbors();
    void CalculateConnectionTimes();
    void HandleClusterHeadReplacement();
    void HandleClusterMemberManagement();

    // Simulation-specific
    void ScheduleNextBeacon();
    void ScheduleClusterMaintenance();

    // Member variables
    uint32_t m_vehicleId;
    uint32_t m_resourceType;
    uint64_t m_resourceCapacity;
    double m_beaconInterval;
    double m_communicationRange;
    double m_clusterHeadTimeout;
    double m_clusterMaintenanceInterval;

    // Data structures
    std::map<uint32_t, RcsaVehicle*> m_vehicles;
    std::map<uint32_t, RcsaCluster*> m_clustersByType;    // Key: resource type
    std::map<uint32_t, RcsaCluster*> m_clustersById;      // Key: cluster ID
    
    // Pending requests for tracking
    struct PendingRequest {
        uint32_t requesterId;
        uint32_t resourceType;
        uint64_t requiredAmount;
        double requiredDuration;
        double requestTime;
    };
    std::map<uint32_t, PendingRequest> m_pendingRequests;

    // Socket for communication
    Ptr<Socket> m_socket;
    Ipv4Address m_localAddress;

    // Event scheduling
    EventId m_beaconEvent;
    EventId m_maintenanceEvent;
    EventId m_clusterConstructionEvent;

    // Statistics
    Statistics m_statistics;

    // Timers for request tracking
    std::map<uint32_t, double> m_requestStartTimes;

    // Call back functions
    void HandleSocketRecv(Ptr<Socket> socket);
};

} // namespace ns3

#endif // RCSA_PROTOCOL_H
