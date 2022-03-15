//
// CollabVM Server
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

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

} // namespace collabvm::websocket

#endif // COLLABVM_HTTPUTILS_H
