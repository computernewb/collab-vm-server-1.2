#pragma once

#if defined(__i686)
	// Define this override only on i686 GCC, as on x86_64 types header already defines this properly
	#define PRIi64    "lli"
#endif

#define HAVE_PNG_GET_IO_PTR
