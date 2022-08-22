//
// CollabVM 3
//
// (C) 2021-2022 CollabVM Development Team
//
// This file is licensed under the GNU General Public License Version 3.
// Text is provided in LICENSE.
//

//
// This file is provided to use a universal "net" namespace
// for ASIO.
//
// In theory, this would allow for compatibility with the
// future C++23 Networking TS, and current experimental implementations
// of said TS.
//
// This file also provides shorthand namespaces for Boost.Beast,
// mostly out of Convenience(TM).
//

#ifndef CORE_HTTP_NETWORKINGTSCOMPATIBILITY_H
#define CORE_HTTP_NETWORKINGTSCOMPATIBILITY_H

// Forward declare some stuff
namespace boost::asio { // NOLINT (this namespace intentionally isn't asio::ip)
	namespace ip {
		class tcp;
	}
} // namespace boost::asio

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

// Addl. Boost.Beast shorthand usage.

namespace boost::beast {
	namespace http {}
	namespace websocket {}
} // namespace boost::beast

namespace beast = boost::beast;
namespace bhttp = beast::http;
namespace bwebsocket = beast::websocket;

#endif // CORE_HTTP_NETWORKINGTSCOMPATIBILITY_H
