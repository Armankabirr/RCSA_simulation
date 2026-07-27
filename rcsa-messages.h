#ifndef RCSA_MESSAGES_H
#define RCSA_MESSAGES_H

#include "ns3/header.h"
#include "ns3/packet.h"
#include <vector>
#include <cstdint>

namespace ns3 {

// Message types as per RCSA paper
enum RcsaMessageType {
    RCSA_BEACON = 0,              // Periodic beacon with vehicle info
    RCSA_RESOURCE_REQUEST = 1,    // Request for resource
    RCSA_CH_MESSAGE = 2,          // Cluster head announcement
    RCSA_JOIN_MESSAGE = 3,        // Request to join cluster
    RCSA_ALLOCATION = 4,          // Resource allocation confirmation
    RCSA_UPDATE = 5,              // RCM resource update
    RCSA_RCH_SELECTION = 6,       // New RCH selection
    RCSA_RCH_CHANGE = 7,          // RCH replacement notification
    RCSA_ACKNOWLEDGE = 8,         // Acknowledgement message
    RCSA_LEAVE = 9                // Leave cluster notification
};

/**
 * Beacon Message Format
 * Contains: ID, mobility, resource type, available resource amount, position
 */
class RcsaBeaconMessage : public Header {
public:
    RcsaBeaconMessage();
    ~RcsaBeaconMessage();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // Setters
    void SetVehicleId(uint32_t id) { m_vehicleId = id; }
    void SetResourceType(uint32_t type) { m_resourceType = type; }
    void SetAvailableResource(uint64_t amount) { m_availableResource = amount; }
    void SetTotalCapacity(uint64_t capacity) { m_totalCapacity = capacity; }
    void SetVelocityX(double vx) { m_velocityX = vx; }
    void SetVelocityY(double vy) { m_velocityY = vy; }
    void SetPositionX(double x) { m_positionX = x; }
    void SetPositionY(double y) { m_positionY = y; }

    // Getters
    uint32_t GetVehicleId() const { return m_vehicleId; }
    uint32_t GetResourceType() const { return m_resourceType; }
    uint64_t GetAvailableResource() const { return m_availableResource; }
    uint64_t GetTotalCapacity() const { return m_totalCapacity; }
    double GetVelocityX() const { return m_velocityX; }
    double GetVelocityY() const { return m_velocityY; }
    double GetPositionX() const { return m_positionX; }
    double GetPositionY() const { return m_positionY; }

private:
    uint32_t m_vehicleId;
    uint32_t m_resourceType;
    uint64_t m_availableResource;
    uint64_t m_totalCapacity;
    double m_velocityX;
    double m_velocityY;
    double m_positionX;
    double m_positionY;
};

/**
 * Resource Request Message
 * Contains: Requester ID, requested resource type, required amount, duration
 */
class RcsaResourceRequestMessage : public Header {
public:
    RcsaResourceRequestMessage();
    ~RcsaResourceRequestMessage();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetRequesterId(uint32_t id) { m_requesterId = id; }
    void SetResourceType(uint32_t type) { m_resourceType = type; }
    void SetRequiredAmount(uint64_t amount) { m_requiredAmount = amount; }
    void SetRequiredDuration(double duration) { m_requiredDuration = duration; }
    void SetHopCount(uint32_t hops) { m_hopCount = hops; }

    uint32_t GetRequesterId() const { return m_requesterId; }
    uint32_t GetResourceType() const { return m_resourceType; }
    uint64_t GetRequiredAmount() const { return m_requiredAmount; }
    double GetRequiredDuration() const { return m_requiredDuration; }
    uint32_t GetHopCount() const { return m_hopCount; }

private:
    uint32_t m_requesterId;
    uint32_t m_resourceType;
    uint64_t m_requiredAmount;
    double m_requiredDuration;
    uint32_t m_hopCount;
};

/**
 * Cluster Head Message
 * Contains: CH ID, Cluster ID, resource type
 */
class RcsaClusterHeadMessage : public Header {
public:
    RcsaClusterHeadMessage();
    ~RcsaClusterHeadMessage();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetClusterHeadId(uint32_t id) { m_clusterHeadId = id; }
    void SetClusterId(uint32_t id) { m_clusterId = id; }
    void SetResourceType(uint32_t type) { m_resourceType = type; }

    uint32_t GetClusterHeadId() const { return m_clusterHeadId; }
    uint32_t GetClusterId() const { return m_clusterId; }
    uint32_t GetResourceType() const { return m_resourceType; }

private:
    uint32_t m_clusterHeadId;
    uint32_t m_clusterId;
    uint32_t m_resourceType;
};

/**
 * Allocation Message
 * Contains: Allocated resource amount, duration, RCM IDs
 */
class RcsaAllocationMessage : public Header {
public:
    RcsaAllocationMessage();
    ~RcsaAllocationMessage();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetAllocatedAmount(uint64_t amount) { m_allocatedAmount = amount; }
    void SetDuration(double duration) { m_duration = duration; }
    void AddAllocatingMemberId(uint32_t id) { m_allocatingMemberIds.push_back(id); }
    void SetRequesterId(uint32_t id) { m_requesterId = id; }

    uint64_t GetAllocatedAmount() const { return m_allocatedAmount; }
    double GetDuration() const { return m_duration; }
    const std::vector<uint32_t>& GetAllocatingMemberIds() const { 
        return m_allocatingMemberIds; 
    }
    uint32_t GetRequesterId() const { return m_requesterId; }

private:
    uint64_t m_allocatedAmount;
    double m_duration;
    std::vector<uint32_t> m_allocatingMemberIds;
    uint32_t m_requesterId;
};

} // namespace ns3

#endif // RCSA_MESSAGES_H
