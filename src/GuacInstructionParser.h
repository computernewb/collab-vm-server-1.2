#pragma once
#include <string>
#include "CollabVMUser.h"
#include <vector>

namespace GuacInstructionParser {
	/**
	 * Max element size of a Guacamole element.
	 */
	constexpr std::uint64_t MAX_GUAC_ELEMENT_LENGTH = 3450;

	/**
	 * Max size of a Guacamole frame.
	 */
	constexpr std::uint64_t MAX_GUAC_FRAME_LENGTH = 6144;

	/**
	 * Parses the instruction from a Guacamole webclient and calls the
	 * handler for it.
	 * @param server The CollabVMServer to call the handler with.
	 * @param data The connection data belonging to the client that sent the instruction.
	 * @param instruction The instruction to parse.
	 */
	void ParseInstruction(CollabVMServer& server, const std::shared_ptr<CollabVMUser>& user, const std::string& instruction);
	std::vector<std::string> DecodeInstruction(const std::string& input,
											   std::uint64_t max_frame_length = GuacInstructionParser::MAX_GUAC_FRAME_LENGTH,
											   std::uint64_t max_element_length = GuacInstructionParser::MAX_GUAC_ELEMENT_LENGTH);
} // namespace GuacInstructionParser
