#include "GuacInstructionParser.h"
#include "CollabVM.h"
#include <vector>

//#include <functional>
#include <string>

namespace GuacInstructionParser {
	struct Instruction {
		const char* opcode;
		CollabVMServer::GuacamoleInstruction handler;
	};

	constexpr static Instruction instructions[] = {
		// Guacamole instructions
		{ "mouse", &CollabVMServer::OnMouseInstruction },
		{ "key", &CollabVMServer::OnKeyInstruction },

		// Custom instructions
		{ "rename", &CollabVMServer::OnRenameInstruction },
		{ "connect", &CollabVMServer::OnConnectInstruction },
		{ "chat", &CollabVMServer::OnChatInstruction },
		{ "turn", &CollabVMServer::OnTurnInstruction },
		{ "admin", &CollabVMServer::OnAdminInstruction },
		{ "nop", &CollabVMServer::OnNopInstruction },
		{ "list", &CollabVMServer::OnListInstruction },
		{ "vote", &CollabVMServer::OnVoteInstruction },
		{ "file", &CollabVMServer::OnFileInstruction },
		{ "version", &CollabVMServer::OnVersionInstruction }
	};

	//constexpr size_t instruction_count = sizeof(instructions)/sizeof(Instruction);

	/**
	 * Max element size of a Guacamole element.
	 */
	//constexpr std::uint64_t MAX_GUAC_ELEMENT_LENGTH = 3450;

	/**
	 * Max size of a Guacamole frame.
	 */
	//constexpr std::uint64_t MAX_GUAC_FRAME_LENGTH = 6144;

	/**
	 * Decode an instruction from a string.
	 * \param[in] input input guacamole string to decode
	 */
	std::vector<std::string> DecodeInstruction(const std::string& input,
											   std::uint64_t max_frame_length,
											   std::uint64_t max_element_length) {
		std::vector<std::string> output;
		if(input.back() != ';')
			return output;

		if(input.empty() || input.length() >= max_frame_length)
			return output;

		std::istringstream iss { input };

		while(iss) {
			unsigned long long length { 0 };
			iss >> length;

			if(!iss)
				return std::vector<std::string>();

			// Ignore weird elements that could be an attempt to crash the server
			if(length >= max_element_length || length >= input.length())
				return std::vector<std::string>();

			// Ignore if there is no period separating data.
			if(iss.peek() != '.')
				return std::vector<std::string>();

			iss.get(); // remove the period

			// Read content
			std::vector<char> content(length + 1, '\0'); // Is there a reason this can't be a string?
			iss.read(content.data(), static_cast<std::streamsize>(length));
			output.emplace_back(content.data(), length); // Safe code(TM)

			const char& separator = iss.peek();
			if(separator != ',') {
				// If the instruction is ending return output
				if(separator == ';')
					return output;

				// Otherwise, this is probably an fuzz attempt,
				// so fuck you!
				return std::vector<std::string>();
			}

			// Eat the character we peeked.
			// I don't see much of the point of this.
			iss.get();
		}
		//
		return std::vector<std::string>();
	}

	void ParseInstruction(CollabVMServer& server, const std::shared_ptr<CollabVMUser>& user, const std::string& instruction) {
		std::vector<std::string> decoded = DecodeInstruction(instruction);
		if(decoded.empty())
			return;

		for(auto& inst : instructions) {
			if(!std::strncmp(inst.opcode, decoded[0].data(), decoded[0].length())) {
				auto argument_conv = std::vector<char*>();

				// Only bother taking this branch if we need to.
				if(decoded.size() > 1) {
					auto argument_begin = &decoded[1]; // this is a bit crumby
					argument_conv.resize(decoded.size() - 1);

					// since the std::string lifetime is managed by us,
					// simply just copy pointers. MAJOR code smell, these should
					// just be taking vector<string_view> or something
					for(size_t j = 0; j < decoded.size() - 1; ++j)
						argument_conv[j] = argument_begin[j].data();
				}

				// if instruction is not nop or version, and version is 0, set it to 1,
				// so the client can't arbitrarily change its version later
				if (user->version == 0 &&
					(std::strncmp("nop", decoded[0].data(), decoded[0].length()) != 0) &&
					(std::strncmp("version", decoded[0].data(), decoded[0].length()) != 0))
					user->version = 1;

				// Call the instruction handler
				(server.*(inst.handler))(user, argument_conv);
				argument_conv.clear();
			}
		}
	}
} // namespace GuacInstructionParser
