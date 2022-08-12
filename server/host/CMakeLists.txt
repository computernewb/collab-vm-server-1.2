# Host executable. Runs all the services in one
# Could be temporary, could not be. We'll find out.

add_executable(collab3-host
		main.cpp
		Arguments.cpp
		)

target_link_libraries(collab3-host PRIVATE
		${CMAKE_THREAD_LIBS_INIT}
		# Boost
		Boost::program_options

		# CollabVM service modules
		collab3::api)

target_include_directories(collab3-host PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3-host __collabvm_gittag)

# make the temporary host server output in the top level of your build dir rather than server/host/
set_target_properties(collab3-host PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
		)

collabvm_targetize(collab3-host)