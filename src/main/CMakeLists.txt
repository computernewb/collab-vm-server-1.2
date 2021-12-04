# Main module CMakeLists

add_executable(collab-vm-server
		main.cpp
		Arguments.cpp
		${PROJECT_SOURCE_DIR}/include/core/PluginAbi.h
)

target_include_directories(collab-vm-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collab-vm-server PRIVATE ${CMAKE_THREAD_LIBS_INIT} Boost::program_options collabvm::core collabvm::websocket )
collabvm_targetize(collab-vm-server)

# TODO: This binary should be the only target which gets installed,
# 	sans plugin-headers.