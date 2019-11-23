#include "GuacInstructionParser.h"
#include "CollabVM.h"
#include <vector>

#include <functional>
#include <string>

namespace GuacInstructionParser
{
	typedef void (CollabVMServer::*InstructionHandler)(const std::shared_ptr<CollabVMUser>&, std::vector<char*>&);

	struct Instruction
	{
		std::string opcode;
		InstructionHandler handler;
	};

	static const Instruction instructions[] = {
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
		{ "file", &CollabVMServer::OnFileInstruction }
	};
	
	constexpr size_t instruction_count = sizeof(instructions)/sizeof(Instruction);

	/**
	 * Max element size of a Guacamole element.
	 */
	constexpr std::uint64_t MAX_GUAC_ELEMENT_LENGTH = 2500;
		
	/**
	 * Max size of a Guacamole frame.
	 */
	constexpr std::uint64_t MAX_GUAC_FRAME_LENGTH = 6144;
		
	 /**
	  * Decode an instruction from a string.
	  * \param[in] input input guacamole string to decode
	  */
	std::vector<std::string> Decode(const std::string& input) 
	{
		std::vector<std::string> output;
		if (input.back() != ';') 
			return output;

		if(input.empty() || input.length() >= MAX_GUAC_FRAME_LENGTH)
			return output;
		
		std::istringstream iss{input};
		while (iss) {
			unsigned long long length{0};
			iss >> length;

			if (!iss) 
				return std::vector<std::string>();
			
			// Ignore weird elements that could be an attempt to crash the server
			if(length >= MAX_GUAC_ELEMENT_LENGTH || length >= input.length())
				return std::vector<std::string>();

			if (iss.peek() != '.') 
				return std::vector<std::string>();

			iss.get();  // remove the period
		
			std::vector<char> content(length + 1, '\0');
			iss.read(content.data(), static_cast<std::streamsize>(length));
			output.push_back(std::string(content.data()));

			const char& separator = iss.peek();
			if (separator != ',') {
				if (separator == ';') 
					return output;
				return std::vector<std::string>();
			}
			iss.get();
		}
		return std::vector<std::string>();
	}

	void ParseInstruction(CollabVMServer& server, const std::shared_ptr<CollabVMUser>& user, const std::string& instruction)
	{
		std::vector<std::string> decoded = Decode(instruction);
		if(decoded.empty()) 
			return;
		
		for(size_t i = 0; i < instruction_count; ++i) {
			if(instructions[i].opcode == decoded[0]) {
				auto arguments = std::vector<std::string>(decoded.begin() + 1, decoded.end());
				auto argument_conv = std::vector<char*>();
				argument_conv.resize(arguments.size());
				for(size_t j = 0; j < arguments.size(); ++j) 
					argument_conv[j] = const_cast<char*>(arguments[j].c_str());
				
				// Call the instruction handler
				InstructionHandler x = instructions[i].handler;
				(server.*x)(user, argument_conv);
				argument_conv.clear();
			}
		}
	}
}
