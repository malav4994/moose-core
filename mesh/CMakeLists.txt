include_directories(../msg)
include_directories(../basecode)
include_directories(../utility)
add_library(mesh 
    ChemCompt.cpp	
    MeshCompt.cpp	
    MeshEntry.cpp	
    CubeMesh.cpp	
    CylBase.cpp	
    CylMesh.cpp
    NeuroNode.cpp	
    NeuroMesh.cpp	
    SpineEntry.cpp
    SpineMesh.cpp
    PsdMesh.cpp
    testMesh.cpp	
    )
