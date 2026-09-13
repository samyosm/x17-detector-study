cmake_minimum_required(VERSION 3.18)
project(x17_detector_study LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)
find_path(TOMLPLUSPLUS_INCLUDE_DIR toml++/toml.h REQUIRED)

add_executable(simulate_detection
    app/main.cc
    src/actions/ActionInitialization.cc
    src/actions/EventAction.cc
    src/actions/RunAction.cc
    src/actions/SteppingAction.cc
    src/configuration/Configuration.cc
    src/detector/DetectorConstruction.cc
    src/source/PrimaryGeneratorAction.cc
    src/source/GunSource.cc
    src/source/PairSource.cc
    src/source/IpcSource.cc
    src/source/X17Source.cc
)
target_compile_features(simulate_detection PRIVATE cxx_std_17)
target_include_directories(simulate_detection PRIVATE src ${TOMLPLUSPLUS_INCLUDE_DIR})
target_link_libraries(simulate_detection PRIVATE
    ${Geant4_LIBRARIES}
)
target_compile_definitions(simulate_detection PRIVATE
    CONFIG_PATH="${PROJECT_SOURCE_DIR}/config/configuration.toml"
)
