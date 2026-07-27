## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    # Build RCSA library
    bld.ns3_module(
        name='rcsa',
        source=[
            'rcsa-vehicle.cc',
            'rcsa-cluster.cc',
            'rcsa-protocol.cc',
        ],
        headers_only=False,
        use='core',
    )

    # Build RCSA simulation
    if bld.env.BUILD_EXAMPLES:
        bld.ns3_program(
            name='rcsa-simulation',
            target='bin/rcsa-simulation',
            source='rcsa-simulation.cc',
            use='core internet wifi aodv flow-monitor applications rcsa',
        )
