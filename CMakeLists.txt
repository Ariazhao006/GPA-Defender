cmake_minimum_required(VERSION 3.20)

project(gpa_defender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

enable_testing()

add_library(gpa_defender_core STATIC
    src/Block.cpp
    src/DefenseTower.cpp
    src/Enemy.cpp
    src/GameEngine.cpp
    src/GameUI.cpp
    src/Obstacle.cpp
    src/PlayerStats.cpp
    src/Questionnaire.cpp
    src/WaveManager.cpp
    src/full_flow_scenario.cpp
)

target_include_directories(gpa_defender_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(gpa_defender_app src/main.cpp)
target_link_libraries(gpa_defender_app PRIVATE gpa_defender_core)

add_executable(backend_smoke
    tests/backend_smoke_main.cpp
    tests/BackendLoopSmokeTest.cpp
)
target_link_libraries(backend_smoke PRIVATE gpa_defender_core)

add_executable(full_flow_smoke tests/full_flow_smoke_main.cpp)
target_link_libraries(full_flow_smoke PRIVATE gpa_defender_core)

add_test(NAME backend_smoke COMMAND backend_smoke)
add_test(NAME full_flow_smoke COMMAND full_flow_smoke)
