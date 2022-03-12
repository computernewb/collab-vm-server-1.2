add_library(collabvm_websocket
		# These files go first because they *will* take the longest time to compile.
		# And it's best to get them done and over with.
		AsioImplementation.cpp
		BeastImplementation.cpp

		Server.cpp
		Listener.cpp
		${PROJECT_SOURCE_DIR}/include/websocket/NetworkingTSCompatibility.h
		HttpSession.cpp
		HttpUtils.h
		${PROJECT_SOURCE_DIR}/include/websocket/Message.h
		${PROJECT_SOURCE_DIR}/include/websocket/Client.h Client.cpp Message.cpp)

target_include_directories(collabvm_websocket PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(collabvm_websocket PRIVATE Boost::system fmt::fmt spdlog::spdlog)



# Tell beast some stuff
target_compile_definitions(collabvm_websocket PUBLIC
		# No more io_service! No more io_context::work!
		-DBOOST_ASIO_NO_DEPRECATED=1

		-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1
		-DBOOST_ASIO_DISABLE_BOOST_BIND=1
		-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1
		-DBOOST_ASIO_DISABLE_BOOST_REGEX=1
		-DBOOST_ASIO_SEPARATE_COMPILATION=1

		# By default, Boost.Beast uses boost::string_view,
		# which has no conversion to the standard library std::string_view type.
		# This is annoying, but thankfully there's an option to make it use
		# the standard library type.
		-DBOOST_BEAST_USE_STD_STRING_VIEW
		-DBOOST_BEAST_SEPARATE_COMPILATION=1
)

target_include_directories(collabvm_websocket PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collabvm_websocket __collabvm_gittag)

add_library(collabvm::websocket ALIAS collabvm_websocket)

collabvm_targetize(collabvm_websocket)