//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

#ifndef CORE_HTTP_HTTPUTILS_H
#define CORE_HTTP_HTTPUTILS_H

#include <boost/beast/http/message.hpp>

#include "NetworkingTSCompatibility.h"
#include <core/Version.h>

namespace collab3::core::http {

	/**
	 * Set any common HTTP response fields.
	 */
	template<class Body, class Fields>
	inline void SetCommonResponseFields(bhttp::response<Body, Fields>& res) {
		res.set(bhttp::field::server, "CollabVM/" COLLAB3_VERSION_TAG);
	}

} // namespace collab3::core::http

#endif // CORE_HTTP_HTTPUTILS_H
