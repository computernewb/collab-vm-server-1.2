# Core library

add_library(collab3_core
		# Header only implementation files
		AsioImplementation.cpp
		http/BeastImplementation.cpp

		config/ConfigStore.cpp

		http/Server.cpp
		http/Listener.cpp
		http/HttpSession.cpp
		http/WebSocketClient.cpp
		http/WebSocketMessage.cpp

#		util/CommandLine.cpp
		)

target_include_directories(collab3_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collab3_core PRIVATE Boost::system)

target_link_libraries(collab3_core PUBLIC
		fmt::fmt
		spdlog::spdlog
		)

target_compile_definitions(collab3_core PUBLIC
		# We always use fmt as a seperate library,
		# so stop spdlog's detection logic right in its tracks.
		-DSPDLOG_FMT_EXTERNAL

		# ASIO controls
		-DBOOST_ASIO_SEPARATE_COMPILATION=1

		# No more io_service! No more io_context::work!
		-DBOOST_ASIO_NO_DEPRECATED=1

		# Disable some weird stuff ASIO can do.
		-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1
		-DBOOST_ASIO_DISABLE_BOOST_BIND=1
		-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1
		-DBOOST_ASIO_DISABLE_BOOST_REGEX=1

		-DBOOST_BEAST_SEPARATE_COMPILATION=1

		# By default, Boost.Beast uses its own string_view,
		# which has no conversion to the standard library std::string_view type.
		# This is annoying, but thankfully there's an option to make it use
		# the standard library type.
		-DBOOST_BEAST_USE_STD_STRING_VIEW
		)

target_include_directories(collab3_core PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3_core __collab3_server_gittag)

# Targetize
collabvm_targetize(collab3_core)

# Add public alias (Use this, please)
add_library(collab3::core ALIAS collab3_core)
