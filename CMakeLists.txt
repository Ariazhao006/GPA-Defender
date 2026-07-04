cmake_minimum_required(VERSION 3.20)

project(gpa_defender LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

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
    src/LevelData.cpp
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

add_executable(level_data_smoke tests/level_data_smoke_main.cpp)
target_link_libraries(level_data_smoke PRIVATE gpa_defender_core)

add_test(NAME backend_smoke COMMAND backend_smoke)
add_test(NAME full_flow_smoke COMMAND full_flow_smoke)
add_test(NAME level_data_smoke COMMAND level_data_smoke)

# ========== GUI Frontend ==========
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps" CACHE PATH
    "Shared dependency cache outside build directories" FORCE)
set(FETCHCONTENT_QUIET OFF CACHE BOOL
    "Show dependency download/configure progress" FORCE)
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://gitee.com/mirrors/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)

add_executable(gpa_defender_gui
    src/gui_main.cpp
    src/frontend/GameFrontend.cpp
    src/frontend/Renderer.cpp
    src/frontend/AudioManager.cpp
    src/frontend/ChestManager.cpp
    src/frontend/EffectManager.cpp
    src/frontend/SettlementVideoDecoder.cpp
    src/frontend/SettlementVideoPlayer.cpp
    src/frontend/TextureManager.cpp
    src/frontend/AssetPaths.cpp
)
target_include_directories(gpa_defender_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend
)
target_compile_definitions(gpa_defender_gui PRIVATE
    GPA_DEFENDER_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(gpa_defender_gui PRIVATE gpa_defender_core raylib)
if(WIN32)
    target_link_libraries(gpa_defender_gui PRIVATE mfplat mfreadwrite mfuuid)
endif()
