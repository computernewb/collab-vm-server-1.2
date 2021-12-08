# PluginCommon library - shared ABI implementation code
# for the CollabVM Plugin Interface

add_library(collabvm_plugincommon
		AbiImplementation.cpp
        )

# Limit plugincommon to include/plugin,
# to simulate a typical installation environment.
target_include_directories(collabvm_plugincommon PUBLIC ${PROJECT_SOURCE_DIR}/include/plugin)

add_library(collabvm::plugin_common ALIAS collabvm_plugincommon)

set_property(TARGET collabvm_plugincommon
		PROPERTY POSITION_INDEPENDENT_CODE ON)

# MAYBE fix?: Should this be targetized? I'm sort of bordering on no,
# but we'll see.
collabvm_targetize(collabvm_plugincommon)