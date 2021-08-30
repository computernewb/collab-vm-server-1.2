add_library(guacamole
    STATIC
        #./client.cpp
        #./parser.cpp
        #./socket-nest.cpp
        #./socket.cpp
        #./user-handlers.cpp
        #./user.cpp
        #./vnc/auth.cpp
        #./vnc/client.cpp
        #./vnc/clipboard.cpp
        #./vnc/cursor.cpp
        #./vnc/display.cpp
        #./vnc/input.cpp
        #./vnc/log.cpp
        #./vnc/settings.cpp
        #./vnc/user.cpp
        #./vnc/vnc.cpp
        ./error.cpp
        ./guac_clipboard.cpp
        ./guac_cursor.cpp
        ./guac_display.cpp
        ./guac_dot_cursor.cpp
        ./guac_iconv.cpp
        ./guac_list.cpp
        ./guac_pointer_cursor.cpp
        ./guac_rect.cpp
        ./guac_string.cpp
        ./guac_surface.cpp
        ./hash.cpp
        ./id.cpp
        ./palette.cpp
        ./pool.cpp
        ./protocol.cpp
        ./timestamp.cpp
        ./unicode.cpp
)

target_link_libraries(guacamole
    PRIVATE
        project_warnings
        project_options
)

set_target_properties(guacamole
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(guacamole
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

if(ENABLE_JPEG)
    target_compile_definitions(guacamole
        PUBLIC
            -DUSE_JPEG
    )
endif()

target_compile_definitions(collab-vm-server PUBLIC -DRAPIDJSON_HAS_CXX11_RVALUE_REFS)
