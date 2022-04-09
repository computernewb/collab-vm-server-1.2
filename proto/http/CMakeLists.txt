add_library(proto_http
        # These files go first because they *will* take the longest time to compile.
        # And it's best to get them done and over with.
        # We should probably maybe move these files to a shared static target
        # for anything in proto/ to reference, when there are multiple proto targets.
        AsioImplementation.cpp
        BeastImplementation.cpp

        Server.cpp
        Listener.cpp
        NetworkingTSCompatibility.h
        HttpSession.cpp
        HttpUtils.h
        Message.h
        Client.h Client.cpp Message.cpp)


target_link_libraries(proto_http PRIVATE
        Boost::system
        collab3::core
        )


target_compile_definitions(proto_http PUBLIC

        # these defines are ditto of the above note on
        # ASIO stuff. We could move them into collab3/core
        -DBOOST_ASIO_SEPARATE_COMPILATION=1
        -DBOOST_BEAST_SEPARATE_COMPILATION=1
        )

target_include_directories(proto_http PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(proto_http __collabvm_gittag)


collabvm_targetize(proto_http)


add_library(collab3::proto_http ALIAS proto_http)