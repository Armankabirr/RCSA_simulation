#include "rcsa-protocol.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/stats-module.h"
#include "ns3/position-allocator.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RcsaSimulation");

class RcsaSimulation {
public:
    RcsaSimulation();
    ~RcsaSimulation();

    void SetNetworkSize(double x, double y) {
        m_networkWidth = x;
        m_networkHeight = y;
    }
    void SetVehicleDensity(uint32_t density) { m_vehicleDensity = density; }
    void SetNumResourceTypes(uint32_t numTypes) { m_numResourceTypes = numTypes; }
    void SetRequesterRatio(double ratio) { m_requesterRatio = ratio; }
    void SetSimulationTime(double time) { m_simulationTime = time; }
    void SetVehicleSpeed(double minSpeed, double maxSpeed) {
        m_minSpeed = minSpeed;
        m_maxSpeed = maxSpeed;
    }

    void Run();
    void PrintResults();

private:
    void CreateNetwork();
    void SetupWifi();
    void SetupMobility();
    void InstallRcsaProtocol();
    void ScheduleRequests();

    double m_networkWidth;
    double m_networkHeight;
    uint32_t m_vehicleDensity;
    uint32_t m_numResourceTypes;
    double m_requesterRatio;
    double m_simulationTime;
    double m_minSpeed;
    double m_maxSpeed;

    NodeContainer m_vehicles;
    NetDeviceContainer m_devices;
    Ipv4InterfaceContainer m_interfaces;
    std::vector<Ptr<RcsaProtocol>> m_rcsaApps;

    struct SimulationResult {
        uint32_t totalVehicles;
        uint32_t totalRequests;
        uint32_t successfulAllocations;
        uint32_t failedAllocations;
        double averageSearchDelay;
        double totalPacketsGenerated;
        double successRatio;
    };
    SimulationResult m_results;
};

RcsaSimulation::RcsaSimulation()
    : m_networkWidth(2000.0),
      m_networkHeight(2000.0),
      m_vehicleDensity(100),
      m_numResourceTypes(3),
      m_requesterRatio(0.3),
      m_simulationTime(100.0),
      m_minSpeed(20.0),
      m_maxSpeed(60.0) {
}

RcsaSimulation::~RcsaSimulation() {
}

void RcsaSimulation::Run() {
    NS_LOG_INFO("Starting RCSA Simulation...");
    NS_LOG_INFO("Network Size: " << m_networkWidth << "x" << m_networkHeight << " m²");
    NS_LOG_INFO("Vehicle Density: " << m_vehicleDensity << " vehicles/km²");
    NS_LOG_INFO("Resource Types: " << m_numResourceTypes);
    NS_LOG_INFO("Requester Ratio: " << m_requesterRatio * 100 << "%");

    CreateNetwork();
    SetupWifi();
    SetupMobility();
    InstallRcsaProtocol();
    ScheduleRequests();

    Simulator::Run();
    Simulator::Destroy();

    PrintResults();
}

void RcsaSimulation::CreateNetwork() {
    NS_LOG_FUNCTION(this);

    double areaInKm2 = (m_networkWidth * m_networkHeight) / (1000.0 * 1000.0);
    uint32_t numVehicles = static_cast<uint32_t>(areaInKm2 * m_vehicleDensity);

    NS_LOG_INFO("Creating " << numVehicles << " vehicles");

    m_vehicles.Create(numVehicles);
    m_results.totalVehicles = numVehicles;

    InternetStackHelper stack;
    stack.Install(m_vehicles);
}

void RcsaSimulation::SetupWifi() {
    NS_LOG_FUNCTION(this);

    YansWifiPhyHelper wifiPhy;
    wifiPhy.Set("TxGain", DoubleValue(1.0));
    wifiPhy.Set("RxGain", DoubleValue(1.0));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                    "Exponent", DoubleValue(3.0));

    Ptr<YansWifiChannel> channel = wifiChannel.Create();
    wifiPhy.SetChannel(channel);

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                  "DataMode", StringValue("OfdmRate6Mbps"),
                                  "ControlMode", StringValue("OfdmRate6Mbps"));

    m_devices = wifi.Install(wifiPhy, wifiMac, m_vehicles);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.0.0");
    m_interfaces = ipv4.Assign(m_devices);

    NS_LOG_INFO("WiFi setup complete. " << m_devices.GetN()
                                         << " devices created.");
}

void RcsaSimulation::SetupMobility() {
    NS_LOG_FUNCTION(this);

    Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator>();
    
    uint32_t gridSize = 20;
    for (uint32_t i = 0; i < m_vehicles.GetN(); i++) {
        double x = (i % gridSize) * (m_networkWidth / gridSize);
        double y = (i / gridSize) * (m_networkHeight / gridSize);
        posAlloc->Add(Vector(x, y, 0));
    }

    MobilityHelper mobility;
    mobility.SetPositionAllocator(posAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(m_vehicles);

    NS_LOG_INFO("Mobility model installed: Constant Velocity");
    NS_LOG_INFO("Speed range: " << m_minSpeed << " - " << m_maxSpeed << " km/h");
}

void RcsaSimulation::InstallRcsaProtocol() {
    NS_LOG_FUNCTION(this);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> typeDist(0, m_numResourceTypes - 1);
    std::uniform_int_distribution<> capacityDist(100, 500);

    for (uint32_t i = 0; i < m_vehicles.GetN(); ++i) {
        Ptr<Node> node = m_vehicles.Get(i);

        uint32_t resourceType = typeDist(gen);
        uint64_t capacity = capacityDist(gen) * 1024 * 1024;

        Ptr<RcsaProtocol> rcsaApp = CreateObject<RcsaProtocol>();
        rcsaApp->SetResourceType(resourceType);
        rcsaApp->SetResourceCapacity(capacity);
        rcsaApp->SetBeaconInterval(0.5);
        rcsaApp->SetCommunicationRange(1000.0);
        rcsaApp->SetClusterHeadTimeout(10.0);

        node->AddApplication(rcsaApp);
        rcsaApp->SetStartTime(Seconds(0.1));
        rcsaApp->SetStopTime(Seconds(m_simulationTime));

        m_rcsaApps.push_back(rcsaApp);
    }

    NS_LOG_INFO("RCSA Protocol installed on all " << m_vehicles.GetN() << " vehicles");
}

void RcsaSimulation::ScheduleRequests() {
    NS_LOG_FUNCTION(this);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> timeDist(1.0, m_simulationTime - 5.0);
    std::uniform_int_distribution<> vehicleDist(0, m_vehicles.GetN() - 1);
    std::uniform_int_distribution<> typeDist(0, m_numResourceTypes - 1);
    std::uniform_int_distribution<> amountDist(50, 200);

    uint32_t numRequesters = static_cast<uint32_t>(m_vehicles.GetN() * m_requesterRatio);

    for (uint32_t i = 0; i < numRequesters; ++i) {
        uint32_t requesterId = vehicleDist(gen);
        double requestTime = timeDist(gen);
        uint32_t requestedType = typeDist(gen);
        uint64_t requestedAmount = static_cast<uint64_t>(amountDist(gen)) * 1024 * 1024;

        auto callback = [this, requesterId, requestedType, requestedAmount]() {
            if (requesterId < m_rcsaApps.size()) {
                m_rcsaApps[requesterId]->IntraResourceSearch(requesterId, requestedType, requestedAmount, 60.0);
            }
        };

        Simulator::Schedule(Seconds(requestTime), callback);
    }

    NS_LOG_INFO("Scheduled " << numRequesters << " requester vehicles");
}

void RcsaSimulation::PrintResults() {
    NS_LOG_FUNCTION(this);

    uint32_t totalBeacons = 0;
    uint32_t totalRequests = 0;
    uint32_t successfulAllocations = 0;
    uint32_t failedAllocations = 0;
    double totalSearchDelay = 0.0;
    uint32_t totalPackets = 0;

    for (const auto& app : m_rcsaApps) {
        RcsaProtocol::Statistics stats = app->GetStatistics();
        totalBeacons += stats.beaconsSent;
        totalRequests += stats.requestsSent;
        successfulAllocations += stats.successfulAllocations;
        failedAllocations += stats.failedAllocations;
        totalSearchDelay += stats.totalSearchDelay;
        totalPackets += stats.totalPacketsGenerated;
    }

    m_results.successfulAllocations = successfulAllocations;
    m_results.failedAllocations = failedAllocations;
    m_results.averageSearchDelay = (totalRequests > 0)
                                       ? (totalSearchDelay / totalRequests)
                                       : 0.0;
    m_results.totalPacketsGenerated = totalPackets;
    m_results.successRatio = (totalRequests > 0)
                                 ? (static_cast<double>(successfulAllocations) / totalRequests)
                                 : 0.0;

    std::cout << "\n========================================\n";
    std::cout << "        RCSA SIMULATION RESULTS\n";
    std::cout << "========================================\n";
    std::cout << "Total Vehicles:           " << m_results.totalVehicles << "\n";
    std::cout << "Total Resource Types:     " << m_numResourceTypes << "\n";
    std::cout << "Simulation Time:          " << m_simulationTime << " s\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Beacons Sent:       " << totalBeacons << "\n";
    std::cout << "Total Requests:           " << totalRequests << "\n";
    std::cout << "Successful Allocations:   " << successfulAllocations << "\n";
    std::cout << "Failed Allocations:       " << failedAllocations << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Average Search Delay:     " << m_results.averageSearchDelay
              << " s\n";
    std::cout << "Total Packets Generated:  " << m_results.totalPacketsGenerated << "\n";
    std::cout << "Success Ratio:            " << m_results.successRatio * 100
              << " %\n";
    std::cout << "========================================\n\n";

    std::ofstream outfile("rcsa-results.txt");
    outfile << "RCSA Simulation Results\n";
    outfile << "Total Vehicles: " << m_results.totalVehicles << "\n";
    outfile << "Total Requests: " << totalRequests << "\n";
    outfile << "Successful Allocations: " << successfulAllocations << "\n";
    outfile << "Failed Allocations: " << failedAllocations << "\n";
    outfile << "Average Search Delay: " << m_results.averageSearchDelay << " s\n";
    outfile << "Total Packets: " << m_results.totalPacketsGenerated << "\n";
    outfile << "Success Ratio: " << m_results.successRatio * 100 << " %\n";
    outfile.close();

    NS_LOG_INFO("Results written to rcsa-results.txt");
}

int main(int argc, char* argv[]) {
    uint32_t vehicleDensity = 100;
    uint32_t numResourceTypes = 3;
    double requesterRatio = 0.3;
    double simTime = 100.0;
    std::string logLevel = "Info";

    CommandLine cmd;
    cmd.AddValue("vehicleDensity", "Vehicle density (vehicles per km²)", vehicleDensity);
    cmd.AddValue("numResourceTypes", "Number of resource types", numResourceTypes);
    cmd.AddValue("requesterRatio", "Ratio of requester vehicles", requesterRatio);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("logLevel", "Log level (All, Debug, Info, Warning, Error)", logLevel);
    cmd.Parse(argc, argv);

    if (logLevel == "All") {
        LogComponentEnable("RcsaSimulation", LOG_LEVEL_ALL);
        LogComponentEnable("RcsaProtocol", LOG_LEVEL_ALL);
    } else if (logLevel == "Debug") {
        LogComponentEnable("RcsaSimulation", LOG_LEVEL_DEBUG);
        LogComponentEnable("RcsaProtocol", LOG_LEVEL_DEBUG);
    } else if (logLevel == "Info") {
        LogComponentEnable("RcsaSimulation", LOG_LEVEL_INFO);
        LogComponentEnable("RcsaProtocol", LOG_LEVEL_INFO);
    }

    RcsaSimulation simulation;
    simulation.SetVehicleDensity(vehicleDensity);
    simulation.SetNumResourceTypes(numResourceTypes);
    simulation.SetRequesterRatio(requesterRatio);
    simulation.SetSimulationTime(simTime);

    simulation.Run();

    return 0;
}
