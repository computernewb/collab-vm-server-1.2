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

find_package(Threads)
find_package(Boost CONFIG REQUIRED system)

# ahh, good ol pkgconfig
# These packages didn't have any nice CMake machinery included with them
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPNG REQUIRED libpng)
pkg_check_modules(ZLIB REQUIRED zlib)
pkg_check_modules(LIBVNCCLIENT REQUIRED libvncclient)
pkg_check_modules(PIXMAN1 REQUIRED pixman-1)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(CAIRO REQUIRED cairo)

target_link_libraries(collab-vm-server
    PRIVATE
        project_warnings
        project_options
        websocketmm
        guacamole
        ${CMAKE_THREAD_LIBS_INIT}
        Boost::boost
        Boost::system
        ${LIBPNG_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${LIBVNCCLIENT_LIBRARIES}
        ${PIXMAN1_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${SQLITE3_LIBRARIES}
        # TODO: FindPackage
        ${CAIRO_LIBRARIES}
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
        ${ZLIB_INCLUDE_DIRS}
        ${LIBVNCCLIENT_INCLUDE_DIRS}
        ${PIXMAN1_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
)

set_target_properties(collab-vm-server
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


target_compile_definitions(collab-vm-server PUBLIC -DRAPIDJSON_HAS_CXX11_RVALUE_REFS)
