add_library(collab3_proto_http
		# This file takes a huge time to compile, so leave it first.
		BeastImplementation.cpp

		Server.cpp
		Listener.cpp
		NetworkingTSCompatibility.h
		HttpSession.cpp
		HttpUtils.h
		WebSocketMessage.h
		WebSocketClient.h WebSocketClient.cpp WebSocketMessage.cpp)


target_link_libraries(collab3_proto_http PRIVATE
		Boost::system
		collab3::core
		)

target_compile_definitions(collab3_proto_http PUBLIC

		-DBOOST_BEAST_SEPARATE_COMPILATION=1

		# By default, Boost.Beast uses boost::string_view,
		# which has no conversion to the standard library std::string_view type.
		# This is annoying, but thankfully there's an option to make it use
		# the standard library type.
		# This is only relevant to projects pulling in proto/http and therefore beast,
		# so, leave it here.
		-DBOOST_BEAST_USE_STD_STRING_VIEW
		)

target_include_directories(collab3_proto_http PRIVATE ${PROJECT_BINARY_DIR})
add_dependencies(collab3_proto_http __collabvm_gittag)


collabvm_targetize(collab3_proto_http)


add_library(collab3::proto_http ALIAS collab3_proto_http)