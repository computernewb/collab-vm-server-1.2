add_library(websocketmm
    STATIC
        ./listener.cpp
        ./server.cpp
        ./websocket_user.cpp
)

target_link_libraries(websocketmm
    PRIVATE
        project_warnings
        project_options
)

set_target_properties(websocketmm
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(websocketmm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
