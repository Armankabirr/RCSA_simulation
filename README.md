# RCSA (Resource Cluster-based Resource Search and Allocation Scheme) Implementation

This is a complete NS-3 implementation of the RCSA scheme as described in:
> "Resource Cluster-Based Resource Search and Allocation Scheme for Vehicular Clouds in Vehicular Ad Hoc Networks"
> Authors: Hyunseok Choi, Yoonhyeong Lee, Gayeong Kim, Euisin Lee, Youngju Nam
> Published: Sensors 2024, 24, 2175

## Project Structure

```
├── rcsa-vehicle.h          # Vehicle class with resource management
├── rcsa-vehicle.cc         # Vehicle implementation
├── rcsa-cluster.h          # Resource Cluster class
├── rcsa-cluster.cc         # Cluster implementation with CH election
├── rcsa-messages.h         # RCSA protocol messages
├── rcsa-protocol.h         # Main RCSA protocol manager
├── rcsa-protocol.cc        # Protocol implementation (Algorithms 1-4)
├── rcsa-simulation.cc      # NS-3 simulation scenario
├── wscript                 # NS-3 build configuration
└── README.md              # This file
```

## Prerequisites

### 1. NS-3 Installation
```bash
# Download NS-3
cd ~
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev

# Configure and build NS-3
./waf configure --build-examples --enable-examples
./waf build
```

### 2. SUMO Installation
```bash
# Ubuntu/Debian
sudo apt-get install sumo sumo-tools sumo-doc

# Or build from source
git clone https://github.com/eclipse/sumo.git
cd sumo
mkdir build && cd build
cmake ..
make
```

### 3. TraCI (Traffic Control Interface)
TraCI comes with SUMO and enables NS-3 to control SUMO vehicles.
The Python TraCI library is already included in SUMO.

## Building RCSA

### Method 1: Copy to NS-3 Directory (Recommended)

```bash
# Copy RCSA files to NS-3 contrib directory
cp *.h *.cc wscript ~/ns-3-dev/src/contrib/rcsa/

# Or create new module
cd ~/ns-3-dev/src
mkdir -p contrib/rcsa
cp /path/to/rcsa/* contrib/rcsa/

# Rebuild NS-3
cd ~/ns-3-dev
./waf configure
./waf build
```

### Method 2: Standalone Build

```bash
# Create build directory
mkdir build_rcsa && cd build_rcsa

# Compile RCSA files with NS-3 headers
g++ -std=c++11 \
    -I$(NS3_PATH)/build/include \
    -I$(NS3_PATH)/build/lib \
    -c rcsa-vehicle.cc -o rcsa-vehicle.o
    
g++ -std=c++11 \
    -I$(NS3_PATH)/build/include \
    -I$(NS3_PATH)/build/lib \
    -c rcsa-cluster.cc -o rcsa-cluster.o
    
g++ -std=c++11 \
    -I$(NS3_PATH)/build/include \
    -I$(NS3_PATH)/build/lib \
    -c rcsa-protocol.cc -o rcsa-protocol.o

# Link simulation
g++ -std=c++11 \
    -o rcsa-simulation rcsa-simulation.cc rcsa-vehicle.o rcsa-cluster.o rcsa-protocol.o \
    -I$(NS3_PATH)/build/include \
    -L$(NS3_PATH)/build/lib \
    -lns3-core -lns3-network -lns3-internet -lns3-wifi -lns3-mobility
```

## Running Simulations

### Basic Simulation (No SUMO Integration)

```bash
# Run with default parameters
./rcsa-simulation

# Run with custom parameters
./rcsa-simulation \
    --vehicleDensity=100 \
    --numResourceTypes=3 \
    --requesterRatio=0.3 \
    --simTime=100 \
    --logLevel=Info
```

### Command-Line Parameters

- **vehicleDensity**: Number of vehicles per km² (default: 100)
  - Range: 50-250 (as per paper)
  
- **numResourceTypes**: Number of different resource types (default: 3)
  - Range: 1-5
  
- **requesterRatio**: Ratio of vehicles requesting resources (default: 0.3 or 30%)
  - Range: 0.05-0.50
  
- **simTime**: Simulation duration in seconds (default: 100)
  - Recommended: 100+ seconds for meaningful results
  
- **logLevel**: Logging verbosity (default: Info)
  - Options: All, Debug, Info, Warning, Error

### Example 1: High Vehicle Density
```bash
./rcsa-simulation --vehicleDensity=250 --simTime=150
```

### Example 2: Many Resource Types
```bash
./rcsa-simulation --numResourceTypes=5 --requesterRatio=0.4
```

### Example 3: Debug Mode with Verbose Output
```bash
./rcsa-simulation --logLevel=Debug --vehicleDensity=50 --simTime=50
```

## SUMO Integration (Advanced)

### Setting up SUMO Vehicle Mobility

1. **Create SUMO Network File (example.net.xml)**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<net version="1.16">
    <location netOffset="0.00,0.00" convBoundary="0.00,0.00,2000.00,2000.00"/>
    
    <!-- Define roads -->
    <edge id="edge1" from="node1" to="node2" numLanes="2" speed="13.9"/>
    <edge id="edge2" from="node2" to="node3" numLanes="2" speed="13.9"/>
    
    <!-- Define junctions -->
    <junction id="node1" type="priority" x="500.00" y="500.00"/>
    <junction id="node2" type="priority" x="1000.00" y="1000.00"/>
    <junction id="node3" type="priority" x="1500.00" y="1500.00"/>
</net>
```

2. **Create SUMO Route File (example.rou.xml)**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<routes>
    <vType id="vehicle" accel="2.6" decel="4.5" sigma="0" length="5" minGap="2.5" maxSpeed="20"/>
    
    <route id="route1" edges="edge1 edge2"/>
    
    <vehicle id="v0" type="vehicle" route="route1" depart="0"/>
    <vehicle id="v1" type="vehicle" route="route1" depart="1"/>
    <!-- Add more vehicles -->
</routes>
```

3. **Create SUMO Configuration (example.sumocfg)**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<configuration>
    <input>
        <net-file value="example.net.xml"/>
        <route-files value="example.rou.xml"/>
    </input>
    <time>
        <begin value="0"/>
        <end value="3600"/>
    </time>
    <processing>
        <time-to-teleport value="600"/>
    </processing>
</configuration>
```

4. **Run SUMO with TraCI**:
```bash
# Start SUMO server
sumo --remote-port 6379 -c example.sumocfg

# In another terminal, run NS-3 simulation configured for TraCI
# (Note: TraCI support requires additional NS-3 configuration)
```

## Understanding the Output

### Console Output
```
========================================
        RCSA SIMULATION RESULTS
========================================
Total Vehicles:           100
Total Resource Types:     3
Simulation Time:          100 s
----------------------------------------
Total Beacons Sent:       12000
Total Requests:           30
Successful Allocations:   24
Failed Allocations:       6
----------------------------------------
Average Search Delay:     0.45 s
Total Packets Generated:  1250
Success Ratio:            80 %
========================================
```

### Key Metrics (as per paper Section 5)

1. **Resource Searching Delay**: Time from resource request to allocation
   - Lower is better
   - Affected by vehicle density, mobility, and number of resource types

2. **Number of Packets**: Total packets generated for resource search
   - Lower indicates efficiency
   - Reduced in RCSA due to no flooding

3. **Success Ratio**: Percentage of successful resource allocations
   - Higher is better
   - Shows system's ability to satisfy requests

### Output File: rcsa-results.txt
Generated in the simulation directory with aggregated statistics.

## Architecture Components

### 1. RcsaVehicle (rcsa-vehicle.h/cc)
- Represents individual vehicles with resources
- Maintains neighbor table (as per paper Section 3.1)
- Calculates connection probabilities (Equations 1-15)
- Manages resource allocation/deallocation

### 2. RcsaCluster (rcsa-cluster.h/cc)
- Manages Resource Clusters for each resource type (Section 4.1)
- Implements Cluster Head election (Section 4.1.2)
- Selects RCMs for resource allocation (Section 4.2.3)
- Handles cluster member management (Section 4.3.2)

### 3. RcsaProtocol (rcsa-protocol.h/cc)
- Main RCSA protocol orchestrator
- Implements Algorithm 1: Cluster Construction
- Implements Algorithm 2: Intra-Resource Search (Section 4.2.1)
- Implements Algorithm 4: Inter-Resource Search (Section 4.2.2)
- Manages beacon broadcasting
- Maintains statistics

### 4. RcsaMessages (rcsa-messages.h)
- Beacon Message: Vehicle resource information (Figure 1)
- Resource Request: Request for resources
- Cluster Head Message: CH announcements
- Allocation Message: Resource allocation
- Various control messages (RCH_Change, Leave, etc.)

### 5. RcsaSimulation (rcsa-simulation.cc)
- NS-3 simulation scenario setup
- Network topology creation
- Mobility model configuration
- Statistics collection

## Key Algorithms Implemented

### Algorithm 1: Resource Cluster Construction
- Section 4.1 in paper
- Groups vehicles by resource type
- Performs single/multi-hop clustering

### Algorithm 1.2: Election of Resource Cluster Headers
- Section 4.1.2 in paper
- Selects RCH based on:
  - Average connection time with neighbors
  - Available resource capacity
  - Mobility characteristics

### Algorithm 2: Intra-Resource Search (Lines 1-11 in Algorithm 3)
- Section 4.2.1 in paper
- Requester searches for same resource type
- Uses RCM neighbors to reach RCH
- RCH allocates resources

### Algorithm 4: Inter-Resource Search
- Section 4.2.2 in paper
- Searches for different resource type
- Two cases:
  - Case 1: RCH-to-RCH communication
  - Case 2: Direct member-to-member discovery

### Resource Allocation (Section 4.2.3)
- Selects minimum number of RCMs
- Prioritizes high connection time
- Minimizes resource waste
- Figure 4 example implemented

## Performance Characteristics (from Paper Figure 5-7)

### Resource Searching Delay
- **vs Vehicle Density**: Improves with density (more neighbors)
- **vs Requester Ratio**: Stable in RCSA vs increases in SERVitES
- **vs Vehicle Speed**: Negligible impact in RCSA
- **vs Resource Types**: Slight increase with more types

### Number of Packets
- **vs Vehicle Density**: Increases linearly
- **vs Requester Ratio**: RCSA maintains lower packet count
- **vs Vehicle Speed**: Decreases with higher speeds
- **vs Resource Types**: Controlled by cluster distribution

### Success Ratio
- **Baseline**: 20-60% improvement over SERVitES (Table 1)
- **Factors**: Vehicle density, resource availability, search coverage

## Troubleshooting

### Issue: Compilation Errors
```bash
# Ensure NS-3 is properly built
cd ~/ns-3-dev
./waf clean
./waf configure --build-examples
./waf build
```

### Issue: Segment Fault at Runtime
```bash
# Run with GDB for debugging
gdb ./rcsa-simulation
(gdb) run --vehicleDensity=50
```

### Issue: No Output Files
```bash
# Check file permissions and disk space
ls -la rcsa-results.txt
du -h .
```

## Configuration for Paper's Experiments

### Experiment 1: Vehicle Density Impact (Figure 5a)
```bash
for density in 25 50 75 100 125 150 175 200 225 250; do
    ./rcsa-simulation --vehicleDensity=$density > results_$density.txt 2>&1
done
```

### Experiment 2: Requester Ratio Impact (Figure 5b)
```bash
for ratio in 0.05 0.10 0.15 0.20 0.25 0.30 0.35 0.40 0.45 0.50; do
    ./rcsa-simulation --requesterRatio=$ratio > results_ratio_$ratio.txt 2>&1
done
```

### Experiment 3: Resource Types Impact (Figure 5d)
```bash
for types in 1 2 3 4 5; do
    ./rcsa-simulation --numResourceTypes=$types > results_types_$types.txt 2>&1
done
```

## Extending the Implementation

### Adding Custom Resource Types
Modify `rcsa-simulation.cc`:
```cpp
// In InstallRcsaProtocol()
std::uniform_int_distribution<> typeDist(0, m_numResourceTypes - 1);
// Adjust type distribution as needed
```

### Implementing Custom Mobility
```cpp
// In SetupMobility()
mobility.SetMobilityModel("ns3::YourCustomModel", ...);
```

### Adding More Metrics
```cpp
// In RcsaProtocol::Statistics struct
// Add new fields for custom metrics
// Update collection in CalculateStatistics()
```

## References

1. Paper: Sensors 2024, Vol. 24, No. 7, 2175
2. NS-3 Documentation: https://www.nsnam.org/
3. SUMO Documentation: https://sumo.dlr.de/
4. IEEE 802.11p Standard

## License

This implementation is provided for academic and research purposes.

## Authors

Implementation by: [Your Name]
Based on: Choi, H.; Lee, Y.; Kim, G.; Lee, E.; Nam, Y. 2024

## Support

For questions or issues:
1. Check README troubleshooting section
2. Review NS-3 documentation
3. Check paper's algorithm descriptions
4. Enable debug logging: `--logLevel=Debug`
