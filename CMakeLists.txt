cmake_minimum_required(VERSION 3.15)
project(collab-vm-server
    LANGUAGES
        CXX C
    HOMEPAGE_URL
        "http://computernewb.com/collab-vm"
    VERSION
        1.2.11
    DESCRIPTION
        "CollabVM is a website where you can control a computer (virtual machine) from your web browser with strangers. Everybody can click on the screen and request an 18 second turn."
)

include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

# Add option for compiler profiling if available
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
    if(ENABLE_BUILD_WITH_TIME_TRACE)
        target_compile_options(project_options INTERFACE -ftime-trace)
    endif()
endif()

# Build-type specific options
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(project_options INTERFACE -g -ggdb -O0 -fpermissive)
    target_compile_definitions(project_options INTERFACE -D_DEBUG -DDEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(project_options INTERFACE -O3 -ffast-math -march=native -mtune=native)
    set_target_properties(project_options PROPERTIES LINK_FLAGS_RELEASE -s) # Strip symbols
elseif(CMAKE_BUILD_TYPE MATCHES "ReleasePortable")
    target_compile_options(project_options INTERFACE -O3 -ffast-math)
    set_target_properties(project_options PROPERTIES LINK_FLAGS_RELEASE -s) # Strip symbols
elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_options(project_options INTERFACE -O3 -ffast-math -g -ggdb)
elseif(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
    target_compile_options(project_options INTERFACE -Os -ffast-math -march=native -mtune=native)
    set_target_properties(project_options PROPERTIES LINK_FLAGS_RELEASE -s) # Strip symbols
    # TODO: Other size optimizations?
    # LTO is a thing but it's up to the user to use via a CMake option
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system if available
include(cmake/Cache.cmake)

# Add linker configuration
include(cmake/Linker.cmake)
configure_linker(project_options)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

add_subdirectory(src)
