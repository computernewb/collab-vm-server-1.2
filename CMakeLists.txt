cmake_minimum_required(VERSION 3.19)
project(collab-vm-server)

include(cmake/CollabVM.cmake)

set(CMAKE_CXX_STANDARD 20)

# Build toggles?

# Dependencies
find_package(Boost REQUIRED COMPONENTS system program_options)

# include module subdirectories.

add_subdirectory(src/main)