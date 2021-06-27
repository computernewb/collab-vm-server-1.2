#pragma once
#include "GuacSocket.h"
#include "UserList.h"

class CollabVMServer;

/**
 * A socket used for broadcasting an instruction to all of the users
 * connected to a GuacClient.
 */
class GuacBroadcastSocket : public GuacSocket {
   public:
	GuacBroadcastSocket(CollabVMServer& server, UserList& users);

	void InstructionBegin() override;
	void InstructionEnd() override;
	void InstructionEnd(bool binary) override;

   private:
	CollabVMServer& server_;
	UserList& users_;
};