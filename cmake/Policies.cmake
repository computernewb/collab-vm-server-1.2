# CMake policy configuration

# Macro to enable new CMake policy.
# Makes this file a *LOT* shorter.
macro (_new_cmake_policy policy)
	if(POLICY ${policy})
		#message(STATUS "Enabling new policy ${policy}")
		cmake_policy(SET ${policy} NEW)
	endif()
endmacro()

_new_cmake_policy(CMP0026) # CMake 3.0: Disallow use of the LOCATION property for build targets.
_new_cmake_policy(CMP0042) # CMake 3.0+ (2.8.12): MacOS "@rpath" in target's install name
_new_cmake_policy(CMP0046) # warn about non-existent dependencies
_new_cmake_policy(CMP0048) # CMake 3.0+: project() command now maintains VERSION
_new_cmake_policy(CMP0054) # CMake 3.1: Only interpret if() arguments as variables or keywords when unquoted.
_new_cmake_policy(CMP0056) # try_compile() linker flags
_new_cmake_policy(CMP0066) # CMake 3.7: try_compile(): use per-config flags, like CMAKE_CXX_FLAGS_RELEASE
_new_cmake_policy(CMP0067) # CMake 3.8: try_compile(): honor language standard variables (like C++11)
_new_cmake_policy(CMP0068) # CMake 3.9+: `RPATH` settings on macOS do not affect `install_name`.
_new_cmake_policy(CMP0075) # CMake 3.12+: Include file check macros honor `CMAKE_REQUIRED_LIBRARIES`
_new_cmake_policy(CMP0077) # CMake 3.13+: option() honors normal variables.