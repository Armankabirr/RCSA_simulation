#include "rcsa-protocol.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include <algorithm>
#include <cmath>
#include "ns3/double.h"

#include "ns3/node-list.h"
#include "ns3/mobility-model.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("RcsaProtocol");
NS_OBJECT_ENSURE_REGISTERED(RcsaProtocol);

TypeId RcsaProtocol::GetTypeId() {
    static TypeId tid = TypeId("ns3::RcsaProtocol")
        .SetParent<Application>()
        .SetGroupName("Rcsa")
        .AddConstructor<RcsaProtocol>()
        .AddAttribute("BeaconInterval", "Time between beacon transmissions",
                      DoubleValue(0.5), MakeDoubleAccessor(&RcsaProtocol::m_beaconInterval),
                      MakeDoubleChecker<double>())
        .AddAttribute("CommunicationRange", "V2V communication range in meters",
                      DoubleValue(200.0),
                      MakeDoubleAccessor(&RcsaProtocol::m_communicationRange),
                      MakeDoubleChecker<double>())
        .AddAttribute("ClusterHeadTimeout", "Timeout for cluster head",
                      DoubleValue(10.0),
                      MakeDoubleAccessor(&RcsaProtocol::m_clusterHeadTimeout),
                      MakeDoubleChecker<double>());
    return tid;
}

RcsaProtocol::RcsaProtocol()
    : m_vehicleId(0xFFFFFFFF),
      m_resourceType(0),
      m_resourceCapacity(0),
      m_beaconInterval(0.5),
      m_communicationRange(200.0),
      m_clusterHeadTimeout(10.0),
      m_clusterMaintenanceInterval(2.0) {
    m_statistics.beaconsSent = 0;
    m_statistics.requestsSent = 0;
    m_statistics.allocationsSent = 0;
    m_statistics.totalSearchDelay = 0.0;
    m_statistics.successfulAllocations = 0;
    m_statistics.failedAllocations = 0;
    m_statistics.totalPacketsGenerated = 0;
}

RcsaProtocol::~RcsaProtocol() {
    NS_LOG_FUNCTION(this);
}

void RcsaProtocol::StartApplication() {
    NS_LOG_FUNCTION(this);

    // Extract vehicle ID from node
    m_vehicleId = GetNode()->GetId();
    NS_LOG_INFO("RCSA Protocol started for vehicle " << m_vehicleId
                                                     << " (Resource Type: "
                                                     << m_resourceType << ")");

    // Register this vehicle
    RegisterVehicle(m_vehicleId, m_resourceType, m_resourceCapacity);

    // Start beacon broadcasting
    ScheduleNextBeacon();

    // Start cluster construction and maintenance
    m_clusterConstructionEvent = Simulator::Schedule(
        Seconds(1.0), &RcsaProtocol::ConstructResourceClusters, this);

    m_maintenanceEvent = Simulator::Schedule(
        Seconds(m_clusterMaintenanceInterval),
        &RcsaProtocol::MaintainClusters, this);
}

void RcsaProtocol::StopApplication() {
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_beaconEvent);
    Simulator::Cancel(m_maintenanceEvent);
    Simulator::Cancel(m_clusterConstructionEvent);
}

void RcsaProtocol::DoDispose() {
    NS_LOG_FUNCTION(this);
    for (auto& pair : m_vehicles) {
        delete pair.second;
    }
    m_vehicles.clear();

    for (auto& pair : m_clustersByType) {
        delete pair.second;
    }
    m_clustersByType.clear();
    m_clustersById.clear();

    Application::DoDispose();
}

void RcsaProtocol::RegisterVehicle(uint32_t vehicleId, uint32_t resourceType,
                                   uint64_t capacity) {
    if (m_vehicles.find(vehicleId) == m_vehicles.end()) {
        RcsaVehicle* vehicle = new RcsaVehicle(vehicleId, resourceType, capacity);
        m_vehicles[vehicleId] = vehicle;
    }
}

void RcsaProtocol::UpdateVehiclePosition(uint32_t vehicleId, double x, double y) {
    auto it = m_vehicles.find(vehicleId);
    if (it != m_vehicles.end()) {
        it->second->SetPosition(Vector(x, y, 0));
    }
}

void RcsaProtocol::UpdateVehicleVelocity(uint32_t vehicleId, double vx, double vy) {
    auto it = m_vehicles.find(vehicleId);
    if (it != m_vehicles.end()) {
        it->second->SetVelocity(Vector(vx, vy, 0));
    }
}

RcsaVehicle* RcsaProtocol::GetVehicle(uint32_t vehicleId) const {
    auto it = m_vehicles.find(vehicleId);
    if (it != m_vehicles.end()) {
        return it->second;
    }
    return nullptr;
}

// Algorithm 1: Resource Cluster Construction
void RcsaProtocol::ConstructResourceClusters() {
    NS_LOG_FUNCTION(this);

    PerformResourceTypeClustering();
    ElectClusterHeads();

    // Schedule next construction
    m_clusterConstructionEvent = Simulator::Schedule(
        Seconds(2.0), &RcsaProtocol::ConstructResourceClusters, this);
}

// Section 4.1.1: Resource Type-Based Clustering
void RcsaProtocol::PerformResourceTypeClustering() {
    // Step 1: Find neighbors within communication range
    FindNeighbors();

    // Step 2: Group vehicles by resource type
    std::map<uint32_t, std::vector<uint32_t>> vehiclesByType;
    for (auto& pair : m_vehicles) {
        RcsaVehicle* vehicle = pair.second;
        uint32_t type = vehicle->GetResourceType();
        vehiclesByType[type].push_back(vehicle->GetId());
    }

    // Step 3: Create or update clusters for each resource type
    for (auto& typePair : vehiclesByType) {
        uint32_t resourceType = typePair.first;
        std::vector<uint32_t>& vehicleIds = typePair.second;

        // Get or create cluster for this resource type
        RcsaCluster* cluster = nullptr;
        if (m_clustersByType.find(resourceType) == m_clustersByType.end()) {
            uint32_t clusterId = resourceType; // Use resource type as cluster ID
            cluster = new RcsaCluster(clusterId, resourceType);
            m_clustersByType[resourceType] = cluster;
            m_clustersById[clusterId] = cluster;
        } else {
            cluster = m_clustersByType[resourceType];
        }

        // Add members to cluster
        for (uint32_t vehicleId : vehicleIds) {
            RcsaVehicle* vehicle = m_vehicles[vehicleId];
            cluster->AddMember(vehicle);
        }
    }

    NS_LOG_INFO("Performed resource type clustering at time "
                << Simulator::Now().GetSeconds() << "s. Created "
                << vehiclesByType.size() << " resource clusters.");
}

// Section 4.1.2: Election of Resource Cluster Headers
void RcsaProtocol::ElectClusterHeads() {
    NS_LOG_FUNCTION(this);

    double timeStep = m_beaconInterval;

    for (auto& pair : m_clustersByType) {
        RcsaCluster* cluster = pair.second;
        if (cluster && !cluster->IsEmpty()) {
            cluster->ElectClusterHead(timeStep, m_communicationRange);
            NS_LOG_INFO("Cluster " << cluster->GetClusterId()
                                    << " elected CH: "
                                    << cluster->GetClusterHeadId());
        }
    }
}

void RcsaProtocol::FindNeighbors() {
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
}

void RcsaProtocol::CalculateConnectionTimes() {
    for (auto& pair : m_vehicles) {
        RcsaVehicle* vehicle = pair.second;
        vehicle->CalculateAverageConnectionTime(m_beaconInterval, m_communicationRange);
    }
}

// Algorithm 2: Intra-Resource Search (Section 4.2.1)
void RcsaProtocol::IntraResourceSearch(uint32_t requesterId,
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
    double messageDelay = 0.2; 
    double searchDelay = messageDelay;
    m_statistics.totalSearchDelay += searchDelay;

    NS_LOG_INFO("Intra-resource allocation successful for vehicle "
                << requesterId << " from " << sameTypeNeighbors.size() 
                << " neighbors. Delay: " << searchDelay << "s. Required: " << requiredAmount << " bytes");
}

// Algorithm 4: Inter-Resource Search (Section 4.2.2)
void RcsaProtocol::InterResourceSearch(uint32_t requesterId,
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

    // Look for neighbors with different resource types
    const auto& neighbors = requester->GetNeighbors();
    bool foundCrossTypeConnection = false;

    for (const auto& neighborPair : neighbors) {
        const NeighborInfo& neighborInfo = neighborPair.second;

        if (neighborInfo.resourceType == resourceType) {
            // This should be handled by intra-resource search
            continue;
        }

        // Check if this neighbor has connections to requested resource type
        RcsaVehicle* neighbor = GetVehicle(neighborInfo.vehicleId);
        if (!neighbor) continue;

        std::vector<uint32_t> connectedTypes =
            neighbor->GetNeighborsWithResourceType(resourceType);

        if (!connectedTypes.empty()) {
            foundCrossTypeConnection = true;
            NS_LOG_INFO("Found cross-type connection through vehicle "
                        << neighbor->GetId());
            break;
        }
    }

    // If no direct connection, try CH-to-CH communication
    RcsaCluster* requestedCluster = GetClusterByType(resourceType);
    if (!requestedCluster || !requestedCluster->IsClusterHeadValid()) {
        NS_LOG_INFO("No valid cluster for requested resource type " << resourceType);
        m_statistics.failedAllocations++;
        return;
    }

    // Select resources from requested cluster
    RcsaCluster::AllocationCandidate allocation =
        requestedCluster->SelectResourceMembers(requiredAmount, requesterId);

    if (allocation.totalResourceProvided >= requiredAmount) {
        m_statistics.successfulAllocations++;

        for (uint32_t rcmId : allocation.selectedMembers) {
            RcsaVehicle* rcm = GetVehicle(rcmId);
            if (rcm) {
                rcm->AllocateResource(
                    requiredAmount / allocation.selectedMembers.size());
            }
        }

        double searchDelay = Simulator::Now().GetSeconds() - requestTime;
        m_statistics.totalSearchDelay += searchDelay;

        NS_LOG_INFO("Inter-resource allocation successful for vehicle "
                    << requesterId << ". Delay: " << searchDelay << "s");
    } else {
        NS_LOG_INFO("Insufficient resources in requested cluster");
        m_statistics.failedAllocations++;
    }
}

RcsaCluster* RcsaProtocol::GetClusterByType(uint32_t resourceType) const {
    auto it = m_clustersByType.find(resourceType);
    if (it != m_clustersByType.end()) {
        return it->second;
    }
    return nullptr;
}

RcsaCluster* RcsaProtocol::GetClusterById(uint32_t clusterId) const {
    auto it = m_clustersById.find(clusterId);
    if (it != m_clustersById.end()) {
        return it->second;
    }
    return nullptr;
}

void RcsaProtocol::SendBeacon() {
    NS_LOG_FUNCTION(this);
    m_statistics.beaconsSent++;

    RcsaVehicle* vehicle = GetVehicle(m_vehicleId);
    if (!vehicle) return;

    RcsaBeaconMessage beacon;
    beacon.SetVehicleId(m_vehicleId);
    beacon.SetResourceType(m_resourceType);
    beacon.SetAvailableResource(vehicle->GetAvailableResource());
    beacon.SetTotalCapacity(vehicle->GetTotalCapacity());
    beacon.SetVelocityX(vehicle->GetVelocity().x);
    beacon.SetVelocityY(vehicle->GetVelocity().y);
    beacon.SetPositionX(vehicle->GetPosition().x);
    beacon.SetPositionY(vehicle->GetPosition().y);

    NS_LOG_DEBUG("Vehicle " << m_vehicleId << " sending beacon with "
                            << vehicle->GetAvailableResource() << " available resource");

    m_statistics.totalPacketsGenerated++;
    ScheduleNextBeacon();
}

void RcsaProtocol::ScheduleNextBeacon() {
    m_beaconEvent = Simulator::Schedule(Seconds(m_beaconInterval),
                                        &RcsaProtocol::SendBeacon, this);
}

void RcsaProtocol::MaintainClusters() {
    NS_LOG_FUNCTION(this);

    // Check all clusters and maintain them (Section 4.3)
    for (auto& pair : m_clustersByType) {
        RcsaCluster* cluster = pair.second;
        if (!cluster) continue;

        // Remove vehicles that are out of communication range
        std::vector<uint32_t> toRemove;
        for (uint32_t memberId : cluster->GetMembers()) {
            RcsaVehicle* member = GetVehicle(memberId);
            if (!member) {
                toRemove.push_back(memberId);
                continue;
            }

            // Check if member is still reachable
            RcsaVehicle* localVehicle = GetVehicle(m_vehicleId);
            if (localVehicle) {
                double dx = member->GetPosition().x - localVehicle->GetPosition().x;
                double dy = member->GetPosition().y - localVehicle->GetPosition().y;
                double distance = std::sqrt(dx * dx + dy * dy);

                if (distance > m_communicationRange * 2) {
                    toRemove.push_back(memberId);
                }
            }
        }

        for (uint32_t memberId : toRemove) {
            cluster->RemoveMember(memberId);
        }

        // Replace cluster head if necessary
        cluster->ReplaceClusterHead(m_beaconInterval, m_communicationRange);
    }

    m_maintenanceEvent = Simulator::Schedule(Seconds(m_clusterMaintenanceInterval),
                                             &RcsaProtocol::MaintainClusters, this);
}

void RcsaProtocol::ResetStatistics() {
    m_statistics.beaconsSent = 0;
    m_statistics.requestsSent = 0;
    m_statistics.allocationsSent = 0;
    m_statistics.totalSearchDelay = 0.0;
    m_statistics.successfulAllocations = 0;
    m_statistics.failedAllocations = 0;
    m_statistics.totalPacketsGenerated = 0;
}

void RcsaProtocol::HandleBeaconMessage(Ptr<Packet> packet, Ipv4Address srcAddr) {
    // Beacon processing logic
}

void RcsaProtocol::HandleResourceRequest(Ptr<Packet> packet, Ipv4Address srcAddr) {
    // Request processing logic
}

void RcsaProtocol::HandleAllocationMessage(Ptr<Packet> packet, Ipv4Address srcAddr) {
    // Allocation processing logic
}

void RcsaProtocol::HandleSocketRecv(Ptr<Socket> socket) {
    // Socket receive handling
}

} // namespace ns3
