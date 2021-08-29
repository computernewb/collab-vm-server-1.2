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
    #./guacamole/client.cpp
    ./guacamole/error.cpp
    ./guacamole/guac_clipboard.cpp
    ./guacamole/guac_cursor.cpp
    ./guacamole/guac_display.cpp
    ./guacamole/guac_dot_cursor.cpp
    ./guacamole/guac_iconv.cpp
    ./guacamole/guac_list.cpp
    ./guacamole/guac_pointer_cursor.cpp
    ./guacamole/guac_rect.cpp
    ./guacamole/guac_string.cpp
    ./guacamole/guac_surface.cpp
    ./guacamole/hash.cpp
    ./guacamole/id.cpp
    ./guacamole/palette.cpp
    #./guacamole/parser.cpp
    ./guacamole/pool.cpp
    ./guacamole/protocol.cpp
    #./guacamole/socket-nest.cpp
    #./guacamole/socket.cpp
    ./guacamole/timestamp.cpp
    ./guacamole/unicode.cpp
    #./guacamole/user-handlers.cpp
    #./guacamole/user.cpp
    #./guacamole/vnc/auth.cpp
    #./guacamole/vnc/client.cpp
    #./guacamole/vnc/clipboard.cpp
    #./guacamole/vnc/cursor.cpp
    #./guacamole/vnc/display.cpp
    #./guacamole/vnc/input.cpp
    #./guacamole/vnc/log.cpp
    #./guacamole/vnc/settings.cpp
    #./guacamole/vnc/user.cpp
    #./guacamole/vnc/vnc.cpp
    ./websocketmm/listener.cpp
    ./websocketmm/server.cpp
    ./websocketmm/websocket_user.cpp
    ./GuacInstructionParser.cpp
    ./CollabVM.cpp
)

target_link_libraries(collab-vm-server
    PRIVATE
        project_warnings
        project_options
        # TODO: FindPackage
        -pthread -lturbojpeg -ljpeg -lboost_system -lpng -lz -lvncclient -lcairo -lpixman-1 -ldl -lsqlite3
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

message(AUTHOR_WARNING "${CMAKE_BINARY_DIR}")

set_target_properties(collab-vm-server
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        # CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
