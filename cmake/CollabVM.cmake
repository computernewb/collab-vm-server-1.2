# CollabVM.cmake: some useful CollabVM cmake functions

# Adds some stuff to targets to give them CollabVM core defines
function(collabvm_targetize target)
	target_compile_definitions(${target} PRIVATE "$<$<CONFIG:DEBUG>:COLLABVM_CORE_DEBUG>")
	# TODO: check if CMAKE_BUILD_TYPE == ASAN, and add ASAN arguments here?
	# Ditto for TSAN
endfunction()