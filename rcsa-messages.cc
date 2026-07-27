#include "rcsa-messages.h"
#include "ns3/log.h"

namespace ns3 {

// ===== RcsaBeaconMessage =====
RcsaBeaconMessage::RcsaBeaconMessage()
    : m_vehicleId(0), m_resourceType(0), m_availableResource(0),
      m_totalCapacity(0), m_velocityX(0), m_velocityY(0),
      m_positionX(0), m_positionY(0) {}

RcsaBeaconMessage::~RcsaBeaconMessage() {}

TypeId RcsaBeaconMessage::GetTypeId() {
    static TypeId tid = TypeId("ns3::RcsaBeaconMessage")
        .SetParent<Header>()
        .AddConstructor<RcsaBeaconMessage>();
    return tid;
}

TypeId RcsaBeaconMessage::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t RcsaBeaconMessage::GetSerializedSize() const {
    return sizeof(m_vehicleId) + sizeof(m_resourceType) + 
           sizeof(m_availableResource) + sizeof(m_totalCapacity) + 4*sizeof(uint32_t);
}

void RcsaBeaconMessage::Serialize(Buffer::Iterator start) const {
    start.WriteU32(m_vehicleId);
    start.WriteU32(m_resourceType);
    start.WriteU64(m_availableResource);
    start.WriteU64(m_totalCapacity);
    start.WriteU32((uint32_t)m_velocityX);
    start.WriteU32((uint32_t)m_velocityY);
    start.WriteU32((uint32_t)m_positionX);
    start.WriteU32((uint32_t)m_positionY);
}

uint32_t RcsaBeaconMessage::Deserialize(Buffer::Iterator start) {
    m_vehicleId = start.ReadU32();
    m_resourceType = start.ReadU32();
    m_availableResource = start.ReadU64();
    m_totalCapacity = start.ReadU64();
    m_velocityX = (double)start.ReadU32();
    m_velocityY = (double)start.ReadU32();
    m_positionX = (double)start.ReadU32();
    m_positionY = (double)start.ReadU32();
    return GetSerializedSize();
}

void RcsaBeaconMessage::Print(std::ostream& os) const {
    os << "RcsaBeacon: vehicleId=" << m_vehicleId
       << " resourceType=" << m_resourceType
       << " available=" << m_availableResource;
}

// ===== RcsaResourceRequestMessage =====
RcsaResourceRequestMessage::RcsaResourceRequestMessage()
    : m_requesterId(0), m_resourceType(0), m_requiredAmount(0),
      m_requiredDuration(0), m_hopCount(0) {}

RcsaResourceRequestMessage::~RcsaResourceRequestMessage() {}

TypeId RcsaResourceRequestMessage::GetTypeId() {
    static TypeId tid = TypeId("ns3::RcsaResourceRequestMessage")
        .SetParent<Header>()
        .AddConstructor<RcsaResourceRequestMessage>();
    return tid;
}

TypeId RcsaResourceRequestMessage::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t RcsaResourceRequestMessage::GetSerializedSize() const {
    return 4 * sizeof(uint32_t) + sizeof(uint64_t);
}

void RcsaResourceRequestMessage::Serialize(Buffer::Iterator start) const {
    start.WriteU32(m_requesterId);
    start.WriteU32(m_resourceType);
    start.WriteU64(m_requiredAmount);
    start.WriteU32((uint32_t)m_requiredDuration);
    start.WriteU32(m_hopCount);
}

uint32_t RcsaResourceRequestMessage::Deserialize(Buffer::Iterator start) {
    m_requesterId = start.ReadU32();
    m_resourceType = start.ReadU32();
    m_requiredAmount = start.ReadU64();
    m_requiredDuration = (double)start.ReadU32();
    m_hopCount = start.ReadU32();
    return GetSerializedSize();
}

void RcsaResourceRequestMessage::Print(std::ostream& os) const {
    os << "RcsaRequest: requesterId=" << m_requesterId
       << " resourceType=" << m_resourceType
       << " required=" << m_requiredAmount;
}

// ===== RcsaClusterHeadMessage =====
RcsaClusterHeadMessage::RcsaClusterHeadMessage()
    : m_clusterHeadId(0), m_clusterId(0), m_resourceType(0) {}

RcsaClusterHeadMessage::~RcsaClusterHeadMessage() {}

TypeId RcsaClusterHeadMessage::GetTypeId() {
    static TypeId tid = TypeId("ns3::RcsaClusterHeadMessage")
        .SetParent<Header>()
        .AddConstructor<RcsaClusterHeadMessage>();
    return tid;
}

TypeId RcsaClusterHeadMessage::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t RcsaClusterHeadMessage::GetSerializedSize() const {
    return 3 * sizeof(uint32_t);
}

void RcsaClusterHeadMessage::Serialize(Buffer::Iterator start) const {
    start.WriteU32(m_clusterHeadId);
    start.WriteU32(m_clusterId);
    start.WriteU32(m_resourceType);
}

uint32_t RcsaClusterHeadMessage::Deserialize(Buffer::Iterator start) {
    m_clusterHeadId = start.ReadU32();
    m_clusterId = start.ReadU32();
    m_resourceType = start.ReadU32();
    return GetSerializedSize();
}

void RcsaClusterHeadMessage::Print(std::ostream& os) const {
    os << "RcsaCH: chId=" << m_clusterHeadId
       << " clusterId=" << m_clusterId;
}

// ===== RcsaAllocationMessage =====
RcsaAllocationMessage::RcsaAllocationMessage()
    : m_allocatedAmount(0), m_duration(0), m_requesterId(0) {}

RcsaAllocationMessage::~RcsaAllocationMessage() {}

TypeId RcsaAllocationMessage::GetTypeId() {
    static TypeId tid = TypeId("ns3::RcsaAllocationMessage")
        .SetParent<Header>()
        .AddConstructor<RcsaAllocationMessage>();
    return tid;
}

TypeId RcsaAllocationMessage::GetInstanceTypeId() const {
    return GetTypeId();
}

uint32_t RcsaAllocationMessage::GetSerializedSize() const {
    return sizeof(m_allocatedAmount) + sizeof(uint32_t)*2 + 
           m_allocatingMemberIds.size() * sizeof(uint32_t);
}

void RcsaAllocationMessage::Serialize(Buffer::Iterator start) const {
    start.WriteU64(m_allocatedAmount);
    start.WriteU32((uint32_t)m_duration);
    start.WriteU32(m_requesterId);
    start.WriteU32(m_allocatingMemberIds.size());
    for (uint32_t id : m_allocatingMemberIds) {
        start.WriteU32(id);
    }
}

uint32_t RcsaAllocationMessage::Deserialize(Buffer::Iterator start) {
    m_allocatedAmount = start.ReadU64();
    m_duration = (double)start.ReadU32();
    m_requesterId = start.ReadU32();
    uint32_t count = start.ReadU32();
    m_allocatingMemberIds.clear();
    for (uint32_t i = 0; i < count; i++) {
        m_allocatingMemberIds.push_back(start.ReadU32());
    }
    return GetSerializedSize();
}

void RcsaAllocationMessage::Print(std::ostream& os) const {
    os << "RcsaAlloc: requesterId=" << m_requesterId
       << " allocated=" << m_allocatedAmount;
}

} // namespace ns3
