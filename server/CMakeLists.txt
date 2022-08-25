cmake_minimum_required(VERSION 3.19)

project(collab3-server
		VERSION ${COLLAB3_VERSION}
		LANGUAGES C CXX
		DESCRIPTION "CollabVM 3.0"
		)

collabvm_add_tag_target(__collab3_server_gittag ${PROJECT_BINARY_DIR}/core)

# Include module subdirectories
add_subdirectory(core)
add_subdirectory(api)

# temporary host for all modules
add_subdirectory(host)

