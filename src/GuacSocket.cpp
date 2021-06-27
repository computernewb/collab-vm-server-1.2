#include "GuacSocket.h"

GuacSocket::GuacSocket()
	: base64_(ss_),
	  binary_(false) {
}

size_t GuacSocket::Write(const void* buf, size_t count) {
	ss_.write((const char*)buf, count);
	return 0;
}

size_t GuacSocket::WriteInt(int64_t i) {
	ss_ << i;
	return 0;
}

size_t GuacSocket::WriteString(const char* str) {
	ss_ << str;
	return 0;
}

void GuacSocket::Flush() {
	// Not used
}
