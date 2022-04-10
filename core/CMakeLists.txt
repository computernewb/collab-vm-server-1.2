# Core library

add_library(collab3_core
		# ASIO implmentation
		AsioImplementation.cpp

		util/CommandLine.cpp
		VncHack.cpp
		PluginManager.cpp

		)

target_include_directories(collab3_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
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

		-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1
		-DBOOST_ASIO_DISABLE_BOOST_BIND=1
		-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1
		-DBOOST_ASIO_DISABLE_BOOST_REGEX=1
		)

# Targetize
collabvm_targetize(collab3_core)

# Add public alias (Use this, please)
add_library(collab3::core ALIAS collab3_core)
