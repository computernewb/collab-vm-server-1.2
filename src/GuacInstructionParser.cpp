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

	/**
	* Reads the length prefix from a guacamole instruction.
	* @param str The beginning of the string.
	* @param len The total length of the string.
	* @param start_index The index to start reading from.
	* @param index A pointer to a size_t that receives the index of the element.
	* @returns The length of the element or -1 if there was a parsing error.
	*/
	static size_t ReadGuacElement(const char* str, size_t len, int start_index, size_t* index)
	{
		size_t element_len = 0;
		for (size_t i = start_index; i < len; i++)
		{
			char c = str[i];
			if (c >= '0' && c <= '9')
			{
				element_len = element_len * 10 + c - '0';
			}
			else if (c == '.')
			{
				i++;
				c = str[i + element_len];
				// Check for a valid terminator
				if (c == ',' || c == ';')
				{
					*index = i;
					return element_len;
				}
				break;
			}
			else
			{
				break;
			}
		}
		return -1;
	}

	void ParseInstruction(CollabVMServer& server, const std::shared_ptr<CollabVMUser>& user, const char* instr, size_t instr_len)
	{
		size_t index;
		size_t len = ReadGuacElement(instr, instr_len, 0, &index);
		if (len == -1)
		{
			// Parse error
			return;
		}

		const char* opcode = &instr[index];
		// Look for the opcode in the instruction array
		int i;
		for (i = 0; i < sizeof(instructions)/sizeof(Instruction); i++)
		{
			if (!instructions[i].opcode.compare(0, std::string::npos, opcode, len))
			{
				goto valid_opcode;
			}
		}
		// Return if the instruction was not found in the array of handlers
		return;

	valid_opcode:
		// Copy the instr to a mutable buffer
		char* instr_buf = new char[instr_len];
		std::memcpy(instr_buf, instr, instr_len);

		// If the instruction was recognized, continue parsing the remaining arguments
		std::vector<char*> arguments;
		char c = instr_buf[index + len];
		while (index + len + 1 < instr_len && c == ',')
		{
			len = ReadGuacElement(instr_buf, instr_len, index + len + 1, &index);
			if (len == -1)
			{
				delete[] instr_buf;
				return;
			}

			// Remember previous value
			c = instr_buf[index + len];
			// Add null terminator after opcode
			instr_buf[index + len] = '\0';

			arguments.push_back(&instr_buf[index]);
		}
		// TODO: Some of these checks may be unnecessary
		if (len == -1 || index + len + 1 != instr_len || c != ';')
		{
			delete[] instr_buf;
			return;
		}

		// Call the instruction handler
		InstructionHandler x = instructions[i].handler;
		(server.*x)(user, arguments);

		delete[] instr_buf;
	}

}
