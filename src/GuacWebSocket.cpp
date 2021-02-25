#include "GuacWebSocket.h"
#include "CollabVM.h"
#include <assert.h>

#include <websocketmm/websocket_user.h>

GuacWebSocket::GuacWebSocket(CollabVMServer* server, websocketmm::websocket_user* handle) :
	server_(server),
	websocket_handle_(handle)
{
}

void GuacWebSocket::InstructionBegin()
{
	// Lock stringstream
	mutex_.lock();
}

void GuacWebSocket::InstructionEnd()
{
	std::string str = ss_.str();

	// Clear stringstream
	ss_.str("");

	// Unlock stringstream
	mutex_.unlock();

	// Check that the message ends with a semicolon
	//assert(str[str.length() - 1] == ';');
	if (str.length() && str[str.length() - 1] == ';')
        websocket_handle_->send(websocketmm::BuildWebsocketMessage(str));
		//server_->SendGuacMessage(websocket_handle_, str);
}
