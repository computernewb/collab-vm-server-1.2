#ifndef COLLABVM_HTTPUTILS_H
#define COLLABVM_HTTPUTILS_H

#include <websocket/NetworkingTSCompatibility.h>

#include <boost/beast/http/message.hpp>

namespace collabvm::websocket {

	/**
	 * Set any common HTTP response fields.
	 */
	template<class Body, class Fields>
	constexpr void SetCommonResponseFields(http::response<Body, Fields>& res) {
		res.set(http::field::server, "collab-vm-server/3.0.0");
	}

} // namespace collabvm::websocket

#endif // COLLABVM_HTTPUTILS_H
