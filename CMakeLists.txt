cmake_minimum_required(VERSION 3.19)

# Prohibit in-source tree builds.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are strictly prohibited. Please see BUILDING.md and use that CMake command line as an example.")
endif()

include(cmake/Policies.cmake)

set(COLLAB3_VERSION 3.0.0)

project(collab3
		VERSION ${COLLAB3_VERSION}
		DESCRIPTION "CollabVM 3.0"
		)

set(COLLAB3_TOP ${PROJECT_SOURCE_DIR})

include(cmake/CollabVM.cmake)

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

# Top level third party dependencies
#add_subdirectory(third_party/capnproto)


add_subdirectory(server)

