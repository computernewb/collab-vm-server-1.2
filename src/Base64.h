#pragma once
#include <sstream>

static const char __guac_socket_BASE64_CHARACTERS[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
	'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '+', '/'
};

class Base64 {
   public:
	Base64(std::ostringstream& ss)
		: ss_(ss),
		  base64_ready_(0) {
	}

	size_t WriteBase64Byte(int buf) {
		int* __ready_buf = base64_ready_buf_;

		int retval;

		__ready_buf[base64_ready_++] = buf;

		/* Flush triplet */
		if(base64_ready_ == 3) {
			retval = WriteBase64Triplet(__ready_buf[0], __ready_buf[1], __ready_buf[2]);
			if(retval < 0)
				return retval;

			base64_ready_ = 0;
		}

		return 1;
	}

	size_t WriteBase64(const void* buf, size_t count) {
		int retval;

		const unsigned char* char_buf = (const unsigned char*)buf;
		const unsigned char* end = char_buf + count;

		while(char_buf < end) {
			retval = WriteBase64Byte(*(char_buf++));
			if(retval < 0) {
				return retval;
			}
		}

		return 0;
	}

	size_t FlushBase64() {
		int retval;

		/* Flush triplet to output buffer */
		while(base64_ready_ > 0) {
			retval = WriteBase64Byte(-1);
			if(retval < 0) {
				return retval;
			}
		}

		return 0;
	}

   private:
	size_t WriteBase64Triplet(int a, int b, int c) {
		std::ostringstream& ss = ss_;

		/* Byte 1 */
		ss << __guac_socket_BASE64_CHARACTERS[(a & 0xFC) >> 2]; /* [AAAAAA]AABBBB BBBBCC CCCCCC */

		if(b >= 0) {
			ss << __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)]; /* AAAAAA[AABBBB]BBBBCC CCCCCC */

			if(c >= 0) {
				ss << __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)]; /* AAAAAA AABBBB[BBBBCC]CCCCCC */
				ss << __guac_socket_BASE64_CHARACTERS[c & 0x3F];							  /* AAAAAA AABBBB BBBBCC[CCCCCC] */
			} else {
				ss << __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2)]; /* AAAAAA AABBBB[BBBB--]------ */
				ss << '=';												  /* AAAAAA AABBBB BBBB--[------] */
			}
		} else {
			ss << __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4)]; /* AAAAAA[AA----]------ ------ */
			ss << "==";												  /* AAAAAA AA----[------]------ */
			//ss << '='; /* AAAAAA AA---- ------[------] */
		}

		/* At this point, 4 bytes have been ss_ */
		if(b < 0)
			return 1;

		if(c < 0)
			return 2;

		return 3;
	}

	std::ostringstream& ss_;

	/**
	* The number of bytes present in the base64 "ready" buffer.
	*/
	int base64_ready_;

	/**
	* The base64 "ready" buffer. Once this buffer is filled, base64 data is
	* flushed to the main write buffer.
	*/
	int base64_ready_buf_[3];
};
