#pragma once
#ifdef _WIN32
	#include "LocalSocketClient-Win.hpp"
#else
	#include "LocalSocketClient-Unix.hpp"
#endif
