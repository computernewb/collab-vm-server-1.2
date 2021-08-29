#pragma once
#include "GuacSocket.h"
#include <memory>

#include "websocketmm/fwd.h"

class CollabVMServer;

/**
 * Forwards instructions to a WebSocket client.
 */
class GuacWebSocket : public GuacSocket {
   public:
	GuacWebSocket(CollabVMServer* server, std::weak_ptr<websocketmm::websocket_user> handle);

	size_t Read(void* buf, size_t count);
	size_t Write(const void* buf, size_t count);
	int Select(int usec_timeout);

	void InstructionBegin() override;
	void InstructionEnd() override;

	CollabVMServer* server_;
	std::weak_ptr<websocketmm::websocket_user> websocket_handle_;
};
