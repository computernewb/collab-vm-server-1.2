# Core collab3 library

add_library(collab3_core

		# ASIO/Beast implementation files
		asio/AsioImplTu.cpp
		asio/BeastImplTu.cpp

		Panic.cpp

		config/ConfigStore.cpp

		# HTTP server codebase
		http/Route.cpp
		)

target_include_directories(collab3_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collab3_core PRIVATE Boost::system)

target_link_libraries(collab3_core PUBLIC
		fmt::fmt
		spdlog::spdlog

		tl::expected

		# no nice alias :(
		ctre
		)

target_compile_definitions(collab3_core PUBLIC
		# There isn't really a point to keeping this disabled or a knob atm, since
		# it is pretty much a free performance gain.
		# Keep it a knob, but keep it on.
		-DCOLLAB3_CORE_SYSTEM_EXECUTOR

		# We always use fmt as a separate library,
		# so stop spdlog's detection logic right in its tracks.
		-DSPDLOG_FMT_EXTERNAL

		# ASIO controls
		-DBOOST_ASIO_SEPARATE_COMPILATION=1

		# No more io_service! No more io_context::work!
		-DBOOST_ASIO_NO_DEPRECATED=1

		# Enable io_uring support (we only support Linux anyways :p)
		-DBOOST_ASIO_DISABLE_EPOLL=1

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
		#
		# (this might be getting phased out soon and the library switching
		#  to boost::string_view with no option, but that converts to the
		#  C++17 standard one no problem, so I personally don't have as much
		#  of a gripe with that.)
		-DBOOST_BEAST_USE_STD_STRING_VIEW
		)

target_include_directories(collab3_core PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3_core __collab3_server_gittag)

collab3_target(collab3_core)

# Add public alias (Use this, please)
add_library(collab3::core ALIAS collab3_core)
