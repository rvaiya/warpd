#ifndef _DBG_H_
#define _DBG_H_
	#if DEBUG
		#define dbg(fmt, ...) fprintf(stderr, "DEBUG: "fmt"\n", ##__VA_ARGS__)
	#else
		#define dbg(...)
	#endif
#endif
