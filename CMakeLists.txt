cmake_minimum_required(VERSION 3.19)

# Catch some invalid things that people could do.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are strictly prohibited. Please see docs/building.md and use that CMake command line as an example.")
endif()

if(MSVC OR WIN32)
	message(FATAL_ERROR "Windows is not supported.")
endif()

include(cmake/Policies.cmake)

# Overall version of the monorepo.
# Adheres to SemVer.
#
# Major - Full ABI bump
# Minor - Feature additions, minor ABI bumps
# Patch - Bugfixes, no ABI bumps
set(COLLAB3_VERSION 3.0.0)

project(collab3
		VERSION ${COLLAB3_VERSION}
		DESCRIPTION "CollabVM 3.0"
		)

set(COLLAB3_TOP ${PROJECT_SOURCE_DIR})

include(cmake/CollabVM.cmake)

# Dependencies
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Top level third party dependencies
#add_subdirectory(third_party/capnproto)

# Subprojects part of the Collab3 monorepo.
# Note that even though these are seperate projects,
# they *require* this driver CMakeLists at the top of the project,
# and cannot be built manually. Don't try.

add_subdirectory(server)

