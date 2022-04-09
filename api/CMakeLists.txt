add_executable(collab3-api
        main.cpp
        Arguments.cpp
        )

target_link_libraries(collab3-api PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        # Boost
        Boost::program_options

        # System deps (unneeded)

        # CollabVM modules
        collab3::core
        collab3::proto_http)

target_include_directories(collab3-api PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3-api __collabvm_gittag)

if (WIN32)
    # Windows is big stupid
    target_link_libraries(collab3-api PRIVATE ws2_32 wsock32)
endif ()

# make the api server output in the top level of your build dir rather than api/
set_target_properties(collab3-api PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        )

collabvm_targetize(collab3-api)