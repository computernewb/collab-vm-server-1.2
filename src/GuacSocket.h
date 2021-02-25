#pragma once
#include <sstream>
#include <mutex>
#include "Base64.h"

class GuacSocket {
   public:
	virtual void InstructionBegin() = 0;
	virtual void InstructionEnd() = 0;

	size_t Write(const void* buf, size_t count);
	size_t WriteInt(int64_t i);
	size_t WriteString(const char* str);

	inline size_t WriteBase64Byte(int buf) {
		return base64_.WriteBase64Byte(buf);
	}

	inline size_t WriteBase64(const void* buf, size_t count) {
		return base64_.WriteBase64(buf, count);
	}

	inline size_t FlushBase64() {
		return base64_.FlushBase64();
	}

	void Flush();

	/**
	 * The stringstream buffer for instructions.
	 */
	std::ostringstream ss_;

	/**
	 * Mutex for stringstream.
	 */
	std::mutex mutex_;

   protected:
	GuacSocket();

   private:
	Base64 base64_;
};