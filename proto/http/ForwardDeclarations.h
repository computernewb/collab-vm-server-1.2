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
// the bhttp layer's types, so that a shared_ptr<bhttp::*>
// doesn't end up pulling in Beast/ASIO headers the millisecond it's used.
//

#ifndef PROTO_HTTP_FORWARDDECLARATIONS_H
#define PROTO_HTTP_FORWARDDECLARATIONS_H

namespace collab3::proto::http {
	struct Server;
	struct WebSocketClient;
	struct WebSocketMessage;
} // namespace collab3::proto::http

#endif // PROTO_HTTP_FORWARDDECLARATIONS_H
