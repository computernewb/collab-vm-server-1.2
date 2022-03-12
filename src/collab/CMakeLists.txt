# Collab library

add_library(collabvm_collab
		dummy.cpp
)

target_include_directories(collabvm_collab PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(collabvm_collab PRIVATE Boost::system)

add_library(collabvm::collab ALIAS collabvm_collab)

collabvm_targetize(collabvm_collab)