# Main module CMakeLists

add_executable(collab-vm-server
		main.cpp
		Arguments.cpp
)

target_include_directories(collab-vm-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collab-vm-server PRIVATE
		${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
		# Boost
		Boost::program_options
		Boost::filesystem
		# System deps
		spdlog::spdlog
		fmt::fmt
		# CollabVM modules
		collabvm::core
		collabvm::websocket)

target_include_directories(collab-vm-server PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab-vm-server __collabvm_gittag)

if(WIN32)
	# Windows is big stupid
	target_link_libraries(collab-vm-server PRIVATE ws2_32 wsock32)
endif()

# make the server output in the top level of your build dir rather than src/main
set_target_properties(collab-vm-server PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

collabvm_targetize(collab-vm-server)

# TODO: This binary should be the only target which gets installed,
# 	sans plugin-headers.