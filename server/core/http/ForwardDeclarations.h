//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

//
// This file provides forward declarations for
// the http layer's types, so that a shared_ptr<http::*>
// doesn't end up pulling in Beast/ASIO headers the millisecond it's used.
//

#ifndef CORE_HTTP_FORWARDDECLARATIONS_H
#define CORE_HTTP_FORWARDDECLARATIONS_H

namespace collab3::core::http {
	struct Server;
	struct WebSocketClient;
	struct WebSocketMessage;
} // namespace collab3::core::http

#endif // CORE_HTTP_FORWARDDECLARATIONS_H
