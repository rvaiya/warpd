#include <windows.h>
#include <stdio.h>

const char *get_data_path(const char *file)
{
	static char path[1024];

	sprintf(path, "%s\\warpd\\%s", getenv("APPDATA"), file);
	return path;
}
