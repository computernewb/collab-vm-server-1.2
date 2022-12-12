# CollabVM.cmake: some useful Collab3 repo cmake functions

# Adds some stuff to targets to give them CollabVM core defines
# and misc. options
function(collab3_target target)
	target_compile_definitions(${target} PRIVATE "$<$<CONFIG:DEBUG>:COLLABVM_CORE_DEBUG>")
	target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR})
	target_compile_features(${target} PUBLIC cxx_std_20)

	set(_COLLAB3_CORE_COMPILE_ARGS -Wall -Wextra -Werror -Wno-restrict -fno-exceptions)

	if("asan" IN_LIST COLLABVM_BUILD_FEATURES)
		# Error if someone's trying to mix asan and tsan together,
		# they aren't compatible.
		if("tsan" IN_LIST COLLABVM_BUILD_FEATURES)
			message(FATAL_ERROR "ASAN and TSAN cannot be mixed into a build.")
		endif()

		message(STATUS "Enabling ASAN for target ${target} because it was in COLLABVM_BUILD_FEATURES")
		target_compile_options(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=address)
		target_link_libraries(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=address)
	endif()

	if("tsan" IN_LIST COLLABVM_BUILD_FEATURES)
		message(STATUS "Enabling TSAN for target ${target} because it was in COLLABVM_BUILD_FEATURES")
		target_compile_options(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=thread)
		target_link_libraries(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=thread)
	endif()

	if("ubsan" IN_LIST COLLABVM_BUILD_FEATURES)
		message(STATUS "Enabling UBSAN for target ${target} because it was in COLLABVM_BUILD_FEATURES")
		target_compile_options(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=undefined)
		target_link_libraries(${target} PRIVATE ${_COLLAB3_CORE_COMPILE_ARGS} -fsanitize=undefined)
	endif()
endfunction()

function(collab3_add_tag_target name output_path)
	add_custom_target(${name}
			COMMAND ${CMAKE_COMMAND} -P ${COLLAB3_TOP}/cmake/GitTag.cmake
			WORKING_DIRECTORY ${output_path}
			SOURCES ${output_path}/Version.h)
	set_source_files_properties(${output_path}/Version.h PROPERTIES GENERATED TRUE)
endfunction()

function(collab3_set_alternate_linker linker)
	find_program(LINKER_EXECUTABLE ld.${COLLABVM_LINKER} ${COLLABVM_LINKER})
	if(LINKER_EXECUTABLE)
		message(STATUS "Collab3: Using ${COLLABVM_LINKER} to link")

		# This is supremely, utterly blegh but I guess it works for a global project-wide linker
		if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 12.0.0)
			add_link_options("-ld-path=${COLLABVM_LINKER}")
		else()
			add_link_options("-fuse-ld=${COLLABVM_LINKER}")
		endif()
	else()
		message(FATAL_ERROR "Linker ${COLLABVM_LINKER} does not exist on your system.")
	endif()
endfunction()