#ifndef COLLABVM_HTTPUTILS_H
#define COLLABVM_HTTPUTILS_H

#include <websocket/NetworkingTSCompatibility.h>

#include <boost/beast/http/message.hpp>

#include <Version.h>

namespace collabvm::websocket {

	/**
	 * Set any common HTTP response fields.
	 */
	template<class Body, class Fields>
	inline void SetCommonResponseFields(http::response<Body, Fields>& res) {
		res.set(http::field::server, "collab-vm-server/" COLLABVM_VERSION_TAG);
	}

} // namespace collabvm::http

#endif // COLLABVM_HTTPUTILS_H
