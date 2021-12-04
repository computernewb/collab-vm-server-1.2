# CollabVM.cmake: some useful CollabVM cmake functions

# Adds some stuff to targets to give them CollabVM core defines
function(collabvm_targetize target)
	target_compile_definitions(${target} PRIVATE "$<$<CONFIG:DEBUG>:COLLABVM_CORE_DEBUG>")
endfunction()