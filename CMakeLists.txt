cmake_minimum_required(VERSION 3.19)

# Prohibit in-source tree builds.
# CMake will be banning this in a bit.
# When we can adjust cmake_minimum_required to that version, remove this block.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are strictly prohibited. Please see BUILDING.md and use that CMake command line as an example.")
endif()

include(cmake/Policies.cmake)

project(collab-vm-server)

include(cmake/CollabVM.cmake)

set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# stuff for git tag stuffs
add_custom_target(__collabvm_gittag
        COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/GitTag.cmake
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        SOURCES ${PROJECT_BINARY_DIR}/Version.h)
set_source_files_properties(${PROJECT_BINARY_DIR}/Version.h PROPERTIES GENERATED TRUE)


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
