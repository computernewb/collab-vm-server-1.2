# Core library

add_library(collabvm_core
		util/CommandLine.cpp
		VncHack.cpp
		VncHack.h
		PluginManager.cpp
)

target_include_directories(collabvm_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(collabvm_core PRIVATE Boost::system)

# everything that links to core needs to see the API headers
target_link_libraries(collabvm_core PUBLIC collabvm::plugin_api)

add_library(collabvm::core ALIAS collabvm_core)

collabvm_targetize(collabvm_core)