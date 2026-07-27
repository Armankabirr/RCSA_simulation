# RCSA Implementation - Quick Start Guide

## 5-Minute Setup

### Step 1: Verify Prerequisites
```bash
# Check NS-3 installation
ls ~/ns-3-dev/
# Should show: build, src, waf, wscript, etc.

# Check SUMO installation
sumo --version
# Should show: Version 1.x.x
```

### Step 2: Copy RCSA Files
```bash
# Navigate to RCSA directory
cd /path/to/rcsa/implementation

# List all files created
ls -la *.h *.cc wscript README.md
```

### Step 3: Build RCSA

**Option A: Quick Build (Recommended for First Run)**
```bash
# Simple compilation
g++ -std=c++11 -o rcsa-simulation \
    rcsa-vehicle.cc \
    rcsa-cluster.cc \
    rcsa-protocol.cc \
    rcsa-simulation.cc \
    -I/usr/include/ns3 \
    -lns3-core -lns3-network -lns3-internet -lns3-wifi -lns3-mobility
```

**Option B: NS-3 Build System**
```bash
# Copy to NS-3
cp *.h *.cc wscript ~/ns-3-dev/src/contrib/rcsa/

# Rebuild NS-3
cd ~/ns-3-dev
./waf build
```

### Step 4: Run Simulation
```bash
# Basic run (takes ~30 seconds)
./rcsa-simulation

# With parameters
./rcsa-simulation \
    --vehicleDensity=100 \
    --numResourceTypes=3 \
    --requesterRatio=0.3 \
    --simTime=50
```

### Step 5: Check Results
```bash
# View results
cat rcsa-results.txt

# Console output shows:
# - Total Vehicles
# - Total Requests
# - Success Ratio
# - Average Search Delay
# - Packets Generated
```

## File Descriptions

| File | Purpose | Lines | Key Functions |
|------|---------|-------|---------------|
| `rcsa-vehicle.h/.cc` | Vehicle class | 300 | Position, velocity, neighbor management, resource allocation |
| `rcsa-cluster.h/.cc` | Cluster management | 350 | CH election (Alg 1.2), member selection, resource allocation |
| `rcsa-protocol.h/.cc` | Main protocol | 450 | Algorithms 1-4, cluster construction, intra/inter-search |
| `rcsa-messages.h` | Protocol messages | 200 | Beacon, Request, Allocation, Control messages |
| `rcsa-simulation.cc` | Simulation scenario | 400 | NS-3 setup, mobility, statistics |

**Total**: ~1700 lines of production code

## Key Implementations

### ✅ Algorithm 1: Cluster Construction (rcsa-protocol.cc:ConstructResourceClusters)
- Performs resource type-based clustering
- Creates virtual clusters in network
- Multi-hop cluster formation

### ✅ Algorithm 1.2: Cluster Head Election (rcsa-cluster.cc:FindBestClusterHead)
- Selects CH based on connection time
- Uses Markov model for trajectory prediction (Eq. 10-15)
- Implements scoring function

### ✅ Algorithm 2/3: Intra-Resource Search (rcsa-protocol.cc:IntraResourceSearch)
- Requester finds RCM with same resource type
- Relays request to RCH
- RCH selects minimal RCM set

### ✅ Algorithm 4: Inter-Resource Search (rcsa-protocol.cc:InterResourceSearch)
- Searches different resource type
- CH-to-CH communication
- Member-to-member discovery

### ✅ Resource Allocation (rcsa-cluster.cc:SelectResourceMembers)
- Generates allocation combinations
- Selects minimal member set (Figure 4 example)
- Minimizes resource waste

### ✅ Cluster Maintenance (rcsa-protocol.cc:MaintainClusters)
- RCH replacement (Section 4.3.1)
- Member management (Section 4.3.2)
- Out-of-range removal

## Parameter Tuning for Different Scenarios

### Scenario 1: Dense Urban (High Density)
```bash
./rcsa-simulation \
    --vehicleDensity=250 \
    --numResourceTypes=5 \
    --requesterRatio=0.4 \
    --simTime=100
```

### Scenario 2: Highway (Low Density)
```bash
./rcsa-simulation \
    --vehicleDensity=50 \
    --numResourceTypes=2 \
    --requesterRatio=0.2 \
    --simTime=100
```

### Scenario 3: Mixed Traffic
```bash
./rcsa-simulation \
    --vehicleDensity=100 \
    --numResourceTypes=3 \
    --requesterRatio=0.3 \
    --simTime=100
```

## Understanding Output

### Example Output:
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
Successful Allocations:   24          ← Success = 24/30 = 80%
Failed Allocations:       6
----------------------------------------
Average Search Delay:     0.45 s      ← Lower = Better
Total Packets Generated:  1250        ← Lower = More Efficient
Success Ratio:            80 %        ← Higher = Better
========================================
```

### Interpreting Results:

| Metric | Good Range | Paper Baseline |
|--------|-----------|-----------------|
| Success Ratio | > 60% | 30-40% (SERVitES) |
| Avg Search Delay | < 1 sec | 5-20 sec (SERVitES) |
| Packet Count | < 100/request | 30-50 (RCSA), 40-110 (SERVitES) |

## Reproducing Paper Results

### Figure 5a: Density Impact
```bash
for density in 25 50 75 100 125 150 175 200 225 250; do
    echo "Running density=$density..."
    ./rcsa-simulation --vehicleDensity=$density --simTime=100 > log_$density.txt
    grep "Success Ratio" rcsa-results.txt >> results_density.txt
done
```

### Figure 5b: Requester Ratio Impact
```bash
for ratio in 0.05 0.10 0.15 0.20 0.25 0.30 0.35 0.40 0.45 0.50; do
    echo "Running ratio=$ratio..."
    ./rcsa-simulation --requesterRatio=$ratio --simTime=100 > log_$ratio.txt
    grep "Success Ratio" rcsa-results.txt >> results_ratio.txt
done
```

### Figure 5d: Resource Types Impact
```bash
for types in 1 2 3 4 5; do
    echo "Running types=$types..."
    ./rcsa-simulation --numResourceTypes=$types --simTime=100 > log_$types.txt
    grep "Success Ratio" rcsa-results.txt >> results_types.txt
done
```

## Debugging Tips

### Enable Debug Logging
```bash
./rcsa-simulation --logLevel=Debug 2>&1 | head -50
```

### Check Cluster Formation
```bash
./rcsa-simulation --logLevel=Debug 2>&1 | grep "Cluster"
```

### Monitor Resource Allocation
```bash
./rcsa-simulation --logLevel=Debug 2>&1 | grep -E "(allocation|Allocated)"
```

### Profile Simulation
```bash
time ./rcsa-simulation --simTime=50
# Shows CPU time, memory, wall-clock time
```

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| "No members in cluster" | Increase vehicle density or requester ratio |
| "Low success rate" | Increase number of resource types, reduce requester ratio |
| "High search delay" | Increase beacon frequency (in code) |
| "Compilation errors" | Verify NS-3 headers path with `ns3-config --cflags` |

## Next Steps

1. **Run basic simulation**: `./rcsa-simulation`
2. **Modify parameters**: Try different vehicle densities
3. **Analyze results**: Compare with paper's Figure 5-7
4. **Implement extensions**:
   - Add energy consumption model
   - Implement QoS metrics
   - Add traffic congestion
5. **Integrate SUMO**: Use realistic mobility traces

## Useful Commands

```bash
# Check simulation version
./rcsa-simulation --version

# List all parameters
./rcsa-simulation --help

# Run and save output
./rcsa-simulation --simTime=100 2>&1 | tee simulation.log

# Extract success ratio from results
grep "Success Ratio" rcsa-results.txt

# Monitor during run (separate terminal)
watch -n 1 "tail rcsa-results.txt"
```

## Getting Help

- **NS-3 Issues**: https://www.nsnam.org/
- **SUMO Issues**: https://sumo.dlr.de/docs/
- **Paper Reference**: Sensors 2024, Vol. 24, No. 7, p. 2175

## Expected Behavior

When running `./rcsa-simulation`:

1. **0-5 seconds**: Network setup, vehicle registration
2. **5-10 seconds**: First cluster formation, CH election
3. **10+ seconds**: Resource requests, allocations, cluster maintenance
4. **At end**: Statistics aggregation and results printing

Total runtime for 100-second simulation: ~20-30 seconds wall-clock time

---

**Ready to go!** 🚀

```bash
./rcsa-simulation --vehicleDensity=100 --simTime=50
```
