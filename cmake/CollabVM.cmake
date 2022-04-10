# CollabVM.cmake: some useful Collab3 repo cmake functions

# Adds some stuff to targets to give them CollabVM core defines
# and misc. options
function(collabvm_targetize target)
	target_compile_definitions(${target} PRIVATE "$<$<CONFIG:DEBUG>:COLLABVM_CORE_DEBUG>")

	# Set up their include so that core/... works as expected
	target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR})

	if("asan" IN_LIST COLLABVM_BUILD_FEATURES)
		# Error if someone's trying to mix asan and tsan together,
		# they aren't compatible.
		if("tsan" IN_LIST COLLABVM_BUILD_FEATURES)
			message(FATAL_ERROR "ASAN and TSAN cannot be mixed into a build.")
		endif()

		message(STATUS "Enabling ASAN for target ${target} because it was in COLLABVM_BUILD_FEATURES")
		target_compile_options(${target} PRIVATE "-fsanitize=address")
		target_link_libraries(${target} PRIVATE "-fsanitize=address")
	endif()

	if("tsan" IN_LIST COLLABVM_BUILD_FEATURES)
		message(STATUS "Enabling TSAN for target ${target} because it was in COLLABVM_BUILD_FEATURES")
		target_compile_options(${target} PRIVATE "-fsanitize=thread")
		target_link_libraries(${target} PRIVATE "-fsanitize=thread")
	endif()

endfunction()