add_executable(collab3-test
		core/ConfigStore.cpp
		)

target_link_libraries(collab3-test PRIVATE
		Catch2::Catch2WithMain
		${CMAKE_THREAD_LIBS_INIT}

		collab3::core
		)

target_include_directories(collab3-test PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3-test __collab3_server_gittag)

collab3_target(collab3-test)

catch_discover_tests(collab3-test)