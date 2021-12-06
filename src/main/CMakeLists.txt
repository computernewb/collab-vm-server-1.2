# Main module CMakeLists

add_executable(collab-vm-server
		main.cpp
		Arguments.cpp
)

target_include_directories(collab-vm-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collab-vm-server PRIVATE ${CMAKE_THREAD_LIBS_INIT} Boost::program_options Boost::filesystem ${CMAKE_DL_LIBS} collabvm::core collabvm::websocket)

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

# Dynamic binaries are SO much fun!
if (WIN32)
add_custom_command(TARGET collab-vm-server POST_BUILD
	COMMAND ${CMAKE_SOURCE_DIR}/scripts/copy_dlls_mw.sh ${CMAKE_HOST_SYSTEM_PROCESSOR} ${CMAKE_BINARY_DIR}
	COMMENT "Copying required libraries to build directory"
)
endif()