cmake_minimum_required(VERSION 3.19)

project(collab3-server
		VERSION ${COLLAB3_VERSION}
		LANGUAGES C CXX
		DESCRIPTION "CollabVM 3.0 Server"
		)

collab3_add_tag_target(__collab3_server_gittag ${PROJECT_BINARY_DIR}/core)

# Include module subdirectories
add_subdirectory(core)

# host for virtual machines (runs out-of-process)
#add_subdirectory(vm_host)

# host for all modules
add_subdirectory(host)


if(BUILD_TESTING)
	add_subdirectory(test)
endif()
