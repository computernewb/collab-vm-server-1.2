# CMake policy configuration

if(POLICY CMP0026)
    cmake_policy(SET CMP0026 NEW)
endif()

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)  # CMake 3.0+ (2.8.12): MacOS "@rpath" in target's install name
endif()

if(POLICY CMP0046)
    cmake_policy(SET CMP0046 NEW)  # warn about non-existed dependencies
endif()

if(POLICY CMP0051)
    cmake_policy(SET CMP0051 NEW)
endif()

if(POLICY CMP0054)  # CMake 3.1: Only interpret if() arguments as variables or keywords when unquoted.
    cmake_policy(SET CMP0054 NEW)
endif()

if(POLICY CMP0056)
    cmake_policy(SET CMP0056 NEW)  # try_compile(): link flags
endif()

if(POLICY CMP0066)
    cmake_policy(SET CMP0066 NEW)  # CMake 3.7: try_compile(): use per-config flags, like CMAKE_CXX_FLAGS_RELEASE
endif()

if(POLICY CMP0067)
    cmake_policy(SET CMP0067 NEW)  # CMake 3.8: try_compile(): honor language standard variables (like C++11)
endif()

if(POLICY CMP0068)
    cmake_policy(SET CMP0068 NEW)  # CMake 3.9+: `RPATH` settings on macOS do not affect `install_name`.
endif()

if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)  # CMake 3.12+: Include file check macros honor `CMAKE_REQUIRED_LIBRARIES`
endif()

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)  # CMake 3.13+: option() honors normal variables.
endif()
