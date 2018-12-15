#include "GuacBroadcastSocket.h"
#include "CollabVM.h"
#include "CollabVMUser.h"
#include <assert.h>

GuacBroadcastSocket::GuacBroadcastSocket(CollabVMServer& server, UserList& users) :
	server_(server),
	users_(users)
{
}

void GuacBroadcastSocket::InstructionBegin()
{
	// Lock stringstream
	mutex_.lock();

	// Clear stringstream
	ss_.str("");
}

void GuacBroadcastSocket::InstructionEnd()
{
	std::string str = ss_.str();

	// Unlock stringstream
	mutex_.unlock();

	// Check that the message ends with a semicolon
	assert(str[str.length() - 1] == ';');

	users_.ForEachUserLock([&](CollabVMUser& user)
	{
		server_.SendGuacMessage(user.guac_user->socket_.websocket_handle_, str);
	});
}
