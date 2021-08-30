# TODO: Glob or break guacamole & websocketmm into own CMake projects
add_executable(collab-vm-server
    ./Database/Database.cpp
    ./GuacBroadcastSocket.cpp
    ./GuacClient.cpp
    ./GuacSocket.cpp
    ./GuacUser.cpp
    ./GuacVNCClient.cpp
    ./GuacWebSocket.cpp
    ./Main.cpp
    ./Sockets/AgentClient.cpp
    ./Sockets/QMPClient.cpp
    ./UnitTests/AgentClientTest.cpp
    ./UnitTests/QMPTest.cpp
    ./VMControllers/QMP.cpp
    ./VMControllers/VMController.cpp
    ./VMControllers/QEMUController.cpp
    ./GuacInstructionParser.cpp
    ./CollabVM.cpp
)

# JPEG support (recommended!)
option(ENABLE_JPEG "Enable JPEG as a means to encode the VM display" ON)

if(ENABLE_JPEG)
    find_package(libjpeg-turbo CONFIG REQUIRED)

    target_sources(collab-vm-server
        PRIVATE
            ./cairo_jpg.c
    )
    target_link_libraries(collab-vm-server
        PRIVATE
            libjpeg-turbo::turbojpeg
            libjpeg-turbo::jpeg
    )

    target_compile_definitions(collab-vm-server
        PUBLIC
            -DUSE_JPEG
    )

    target_sources(collab-vm-server
        PRIVATE
            ./cairo_jpg.c
    )
endif()

add_subdirectory(websocketmm)
add_subdirectory(guacamole)

target_link_libraries(collab-vm-server
    PRIVATE
        project_warnings
        project_options
        websocketmm
        guacamole
        # TODO: FindPackage
        -pthread -lboost_system -lpng -lz -lvncclient -lpixman-1 -ldl -lsqlite3 -lcairo
)

target_include_directories(collab-vm-server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/vendor/rapidjson/include
        ${CMAKE_SOURCE_DIR}/vendor/sqlite_orm/include
        ${CMAKE_SOURCE_DIR}/src/Database
        ${CMAKE_SOURCE_DIR}/src/VMControllers
        ${CMAKE_SOURCE_DIR}/src/Sockets
        ${CMAKE_SOURCE_DIR}/src/guacamole
        ${CMAKE_SOURCE_DIR}/src/cpr
        ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(collab-vm-server
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


target_compile_definitions(collab-vm-server PUBLIC -DRAPIDJSON_HAS_CXX11_RVALUE_REFS)
