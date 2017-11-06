#include <re_types.h> 
#include <ctype.h>
#include <stdlib.h>
#include <re_sys.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>

char* sys_time(char* buf, int size) {
	struct timeb timebuffer;
	int len;
	ftime( &timebuffer );
	len = strftime(buf, size, "%T", localtime(&timebuffer.time));
#ifdef _WIN32
    _snprintf(buf + len, size-len, ".%03hu", timebuffer.millitm);
#else
    snprintf(buf + len, size-len, ".%03hu", timebuffer.millitm);
#endif
	buf[size-1] = '\0';
	return buf;
}

