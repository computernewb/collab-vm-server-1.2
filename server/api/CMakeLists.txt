add_library(collab3-api STATIC
		dummy.cpp
		)

target_link_libraries(collab3-api PUBLIC
		# Collab3 modules needed by this project
		collab3::core
)

target_include_directories(collab3-api PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3-api __collab3_server_gittag)

collab3_target(collab3-api)

add_library(collab3::api ALIAS collab3-api)