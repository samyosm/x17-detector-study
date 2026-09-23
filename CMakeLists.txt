cmake_minimum_required(VERSION 3.18)
project(x17_detector_study LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)
find_path(TOMLPLUSPLUS_INCLUDE_DIR toml++/toml.h REQUIRED)

add_library(detector_core STATIC
    src/application/Simulation.cc
    src/application/UiCommand.cc
    src/application/actions/ActionInitialization.cc
    src/application/actions/EventAction.cc
    src/application/actions/PrimaryGeneratorAction.cc
    src/application/actions/RunAction.cc
    src/application/actions/SteppingAction.cc
    src/configuration/Configuration.cc
    src/geometry/CosmicSourcePlane.cc
    src/geometry/DetectorConstruction.cc
    src/geometry/DetectorLayout.cc
    src/output/CosmicWriter.cc
    src/output/EventWriter.cc
    src/output/RootSchema.cc
    src/output/RunFiles.cc
    src/physics/CosmicMuonSpectrum.cc
    src/physics/CosmicObservables.cc
    src/physics/OpticalMaterials.cc
    src/physics/PairDecay.cc
    src/physics/PairObservables.cc
    src/physics/PhysicsList.cc
    src/physics/sources/CosmicMuonSource.cc
    src/physics/sources/GunSource.cc
    src/physics/sources/IpcSource.cc
    src/physics/sources/MixedSource.cc
    src/physics/sources/PairSource.cc
    src/physics/sources/SourceFactory.cc
    src/physics/sources/X17Source.cc
    src/readout/CosmicReadout.cc
    src/readout/OpticalSensor.cc
    src/visualization/CosmicSourceVisualization.cc
    src/visualization/DetectorStyle.cc
    src/visualization/VisualizationSession.cc
)
target_compile_features(detector_core PUBLIC cxx_std_17)
target_include_directories(detector_core PUBLIC src ${TOMLPLUSPLUS_INCLUDE_DIR})
target_link_libraries(detector_core PUBLIC
    ${Geant4_LIBRARIES}
)
add_executable(simulate_detection app/main.cc)
target_link_libraries(simulate_detection PRIVATE detector_core)
target_compile_definitions(simulate_detection PRIVATE
    CONFIG_PATH="${PROJECT_SOURCE_DIR}/config/configuration.toml"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(core_checks tests/core_checks.cc)
    target_link_libraries(core_checks PRIVATE detector_core)
    target_compile_definitions(core_checks PRIVATE PROJECT_PATH="${PROJECT_SOURCE_DIR}")
    add_test(NAME core_checks COMMAND core_checks)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME geometry_check COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/geometry_check.py)
endif()
