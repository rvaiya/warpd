#include "windows.h"

struct mon_thread_args {
	char path[1024];
	DWORD ptid;
};

static const char *dirname(const char *path)
{
	static char dir[1024];
	char *c = dir;
	char *sep = NULL;

	strcpy(dir, path);
	while (*c) {
		if (*c == '\\')
			sep = c;
		c++;
	}

	if (sep)
		*sep = 0;

	return dir;
}

static uint64_t mtime(const char *path)
{
	FILETIME time;
	HANDLE fh = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	assert (fh != INVALID_HANDLE_VALUE);

	GetFileTime(fh, NULL, NULL, &time);

	CloseHandle(fh);

	return (uint64_t)time.dwHighDateTime << 32 | time.dwLowDateTime;
}

static DWORD WINAPI mon_thread(void *arg)
{
	struct mon_thread_args *args = arg;
	const char *dir = dirname(args->path);


	/* Monitor the parent directory. */
	HANDLE mon = FindFirstChangeNotificationA(dir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	assert (mon != INVALID_HANDLE_VALUE);

	uint64_t last_time = mtime(args->path);

	while (1) {
		uint64_t time;

		assert(WaitForSingleObject(mon, INFINITE) == WAIT_OBJECT_0);
		FindNextChangeNotification(mon);

		Sleep(100); /* Ugly hack to accommodate editors (e.g vim) which unlink the file before recreating it. */
		time = mtime(args->path);

		if (last_time != time)
			PostThreadMessage(args->ptid, WM_FILE_UPDATED, 0, 0);

		last_time = time;
	}
}

/*
 * Monitors the supplied path and posts WM_FILE_UPDATED to
 * the calling thread's message queue if the file is modified.
 */

void wn_monitor_file(const char *path)
{
	int ret;
	struct mon_thread_args *args = malloc(sizeof(struct mon_thread_args));

	ret = GetFullPathNameA(path, sizeof args->path, args->path, NULL);

	assert(ret);

	args->ptid = GetCurrentThreadId();
	snprintf(args->path, sizeof args->path, "%s", path);

	CreateThread(0, 0, mon_thread, args, 0, NULL);
}
