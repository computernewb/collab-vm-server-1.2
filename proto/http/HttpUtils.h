#ifndef COLLABVM_HTTPUTILS_H
#define COLLABVM_HTTPUTILS_H

#include <boost/beast/http/message.hpp>

#include "NetworkingTSCompatibility.h"
#include "Version.h"

namespace collab3::proto_http {

	/**
	 * Set any common HTTP response fields.
	 */
	template<class Body, class Fields>
	inline void SetCommonResponseFields(http::response<Body, Fields>& res) {
		res.set(http::field::server, "CollabVM3/" COLLAB3_VERSION_TAG);
	}

} // namespace collab3::proto_http

#endif // COLLABVM_HTTPUTILS_H
