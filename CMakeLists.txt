cmake_minimum_required(VERSION 3.19)

# Prohibit in-source tree builds.
# CMake will be banning this in a bit.
# When we can adjust cmake_minimum_required to that version,
# remove this block
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are strictly prohibited. Please see BUILDING.md and use that CMake command line as an example.")
endif()

include(cmake/Policies.cmake)

project(collab-vm-server)

include(cmake/CollabVM.cmake)

set(CMAKE_CXX_STANDARD 20)

# Build toggles?

# Dependencies
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)

# Include module subdirectories
add_subdirectory(src/core)
add_subdirectory(src/websocket)
add_subdirectory(src/main)