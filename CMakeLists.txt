cmake_minimum_required(VERSION 3.19)

# TODO: Move this to server/
# Move vcpkg.json there too

# Prohibit in-source tree builds.
# CMake will be banning this in a bit.
# When we can adjust cmake_minimum_required to that version, remove this block.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are strictly prohibited. Please see BUILDING.md and use that CMake command line as an example.")
endif()

include(server/cmake/Policies.cmake)

project(collab-vm-server)

include(server/cmake/CollabVM.cmake)

# TODO: collabvm_targetize should set this as a per-project param.
set(CMAKE_CXX_STANDARD 20)

# catch any attempt to compile for windows, whether it be VS or mingw
if(MSVC OR WIN32)
	message(FATAL_ERROR "Windows is not supported.")
endif()

# Dependencies
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Git tag target.
# TODO: Generate in ${PROJECT_BINARY_DIR}/core/ probably
add_custom_target(__collabvm_gittag
		COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/server/cmake/GitTag.cmake
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/server/core
		SOURCES ${PROJECT_BINARY_DIR}/Version.h)
set_source_files_properties(${PROJECT_BINARY_DIR}/Version.h PROPERTIES GENERATED TRUE)


# Include module subdirectories
add_subdirectory(server/core)
add_subdirectory(server/proto)
add_subdirectory(server/api)

# temporary host for all modules
add_subdirectory(server/host)

