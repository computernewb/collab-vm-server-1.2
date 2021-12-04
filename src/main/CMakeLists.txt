# Main module CMakeLists

add_executable(collab-vm-server
		main.cpp
		Arguments.cpp
		${PROJECT_SOURCE_DIR}/include/core/PluginAbi.h
)

target_include_directories(collab-vm-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
	target_link_libraries(collab-vm-server PRIVATE ${CMAKE_THREAD_LIBS_INIT} Boost::program_options collabvm::core collabvm::websocket wsock32 ws2_32)
else() #honestly i find cmake worse for libraries but if its "better than mkconfig", then whatever
	target_link_libraries(collab-vm-server PRIVATE ${CMAKE_THREAD_LIBS_INIT} Boost::program_options collabvm::core collabvm::websocket )
endif()
collabvm_targetize(collab-vm-server)

# TODO: This binary should be the only target which gets installed,
# 	sans plugin-headers.