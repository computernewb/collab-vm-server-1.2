//
// CollabVM Server
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

#ifndef PIXELBOARD_FORWARDDECLARATIONS_H
#define PIXELBOARD_FORWARDDECLARATIONS_H

namespace collabvm::websocket {
	struct Server;
	struct Client;
	struct Message;
} // namespace collabvm::http

#endif // PIXELBOARD_FORWARDDECLARATIONS_H
