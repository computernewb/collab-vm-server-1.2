cmake_minimum_required(VERSION 3.19)
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