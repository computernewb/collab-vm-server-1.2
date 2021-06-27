#include "GuacBroadcastSocket.h"
#include "CollabVM.h"
#include "CollabVMUser.h"
#include <assert.h>
#include "GuacInstructionParser.h"
#include "base64/base64.h"
#include <string>

#include <websocketmm/websocket_user.h>

GuacBroadcastSocket::GuacBroadcastSocket(CollabVMServer& server, UserList& users)
	: server_(server),
	  users_(users) {
}

void GuacBroadcastSocket::InstructionBegin() {
	// Lock stringstream
	mutex_.lock();

	// Clear stringstream
	ss_.str("");
}

void GuacBroadcastSocket::InstructionEnd() {
	std::string str = ss_.str();

	// Unlock stringstream
	mutex_.unlock();

	// Check that the message ends with a semicolon
	assert(str[str.length() - 1] == ';');

	std::vector<std::string> decoded = GuacInstructionParser::DecodeInstruction(str, 4194304, 4194304); // ehh
	if(decoded.size() == 0) return; // in case
	bool isPng = std::strncmp("png", decoded[0].data(), decoded[0].length()) == 0;
	std::string binarystr;
	users_.ForEachUserLock([&](CollabVMUser& user) {
		//user.guac_user->socket_.websocket_handle_->send(websocketmm::BuildWebsocketMessage(str))

		// This really shouldn't happen, but if it does, it does.
		if(user.guac_user != nullptr) {
			if (!isPng)
				server_.SendGuacMessage(user.guac_user->socket_.websocket_handle_, str);
			else {
				if(user.version & UnderVal(Version::BinaryDisplay)) { // user has binary display
					if(binarystr == "") {
						// parse guac, convert png base64 to binary then remake guac... this isn't ideal But-
						// todo maybe have guac_protocol_send_png write binary first then convert to base64 when needed?
						assert(decoded.size() > 5);
						decoded[5] = base64_decode(decoded[5]);
						for (std::string part : decoded) {
							if(binarystr != "") binarystr += ',';
							binarystr += std::to_string(part.length());
							binarystr += '.';
							binarystr += part;
						}
						binarystr += ';';
					}
					server_.SendGuacMessage(user.guac_user->socket_.websocket_handle_, (std::uint8_t*)&binarystr[0], (size_t)(binarystr.length()));
				} else { // base64

					server_.SendGuacMessage(user.guac_user->socket_.websocket_handle_, str);
				}
			}
		}
	});
}

void GuacBroadcastSocket::InstructionEnd(bool binary) {
	if(!binary) {
		InstructionEnd();
		return;
	}
	// i dont think this part is actually gonna ever be reached?
	assert(false);
	std::string str = ss_.str();

	// Unlock stringstream
	mutex_.unlock();

	// Check that the message ends with a semicolon
	assert(str[str.length() - 1] == ';');

	users_.ForEachUserLock([&](CollabVMUser& user) {
		//user.guac_user->socket_.websocket_handle_->send(websocketmm::BuildWebsocketMessage(str))

		// This really shouldn't happen, but if it does, it does.
		if(user.guac_user != nullptr)
			server_.SendGuacMessage(user.guac_user->socket_.websocket_handle_, (std::uint8_t*)&str[0], (size_t)str.length());
	});
}
