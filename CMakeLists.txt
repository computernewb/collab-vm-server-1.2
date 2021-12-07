cmake_minimum_required(VERSION 3.19)

# Prohibit in-source tree builds.
# CMake will be banning this in a bit.
# When we can adjust cmake_minimum_required to that version, remove this block.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are strictly prohibited. Please see BUILDING.md and use that CMake command line as an example.")
endif()

include(cmake/Policies.cmake)

# example: 3.0.0-0 = Server 3.0.0, plugin interface/protocol version 0? thought this would be useful
set(COLLABVM_VERSION_MAJOR 3)
set(COLLABVM_VERSION_MINOR 0)
set(COLLABVM_VERSION_PATCH 0)
set(COLLABVM_PINT_VERSION 1)
set(COLLABVM_VERSION "${COLLABVM_VERSION_MAJOR}.${COLLABVM_VERSION_MINOR}.${COLLABVM_VERSION_PATCH}-${COLLABVM_PINT_VERSION}")

project(collab-vm-server VERSION {COLLABVM_VERSION} LANGUAGES C CXX)

include(cmake/CollabVM.cmake)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Include module subdirectories
add_subdirectory(src/core)
add_subdirectory(src/websocket)
add_subdirectory(src/main)

# Add plugin common code and sample plugins
# if requested in the CMake invocation
if("sampleplugins" IN_LIST COLLABVM_BUILD_FEATURES)
    message(STATUS "Including sample plugin source code into the build")
    add_subdirectory(src/plugincommon)
    add_subdirectory(src/sampleplugins)
endif()
