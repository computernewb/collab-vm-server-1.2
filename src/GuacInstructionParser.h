#pragma once
#include <string>
#include "CollabVMUser.h"

namespace GuacInstructionParser {
	/**
	 * Parses the instruction from a Guacamole webclient and calls the
	 * handler for it.
	 * @param server The CollabVMServer to call the handler with.
	 * @param data The connection data belonging to the client that sent the instruction.
	 * @param instruction The instruction to parse.
	 */
	void ParseInstruction(CollabVMServer& server, const std::shared_ptr<CollabVMUser>& user, const std::string& instruction);
} // namespace GuacInstructionParser
