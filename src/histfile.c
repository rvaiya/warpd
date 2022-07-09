#include "warpd.h"

static struct {
	int sz;
	struct histfile_ent ents[MAX_HIST_ENTS];
} hist;

static void read_hist(const char *path)
{
	int fd = open(path, O_RDWR|O_CREAT, 0600);
	if (fd < 0) {
		perror("open");
		exit(-1);
	}

	hist.sz = 0;
	read(fd, &hist, sizeof(hist));
	close(fd);
}

size_t histfile_read(struct histfile_ent **entries)
{
	read_hist(get_data_path("history"));

	*entries = hist.ents;
	return hist.sz;
}

void histfile_add(int x, int y)
{
	int i, n;
	const char *histpath = get_data_path("history");

	read_hist(histpath);

	n = 0;
	for (i = 0; i < hist.sz; i++)
		if (!((abs(hist.ents[i].x - x) < 30) && 
			(abs(hist.ents[i].y - y) < 30)))
			hist.ents[n++] = hist.ents[i];

	if (n == MAX_HIST_ENTS) {
		memmove(hist.ents, hist.ents+1, sizeof(hist.ents[0])*(MAX_HIST_ENTS-1));
		n--;
	}

	hist.ents[n].x = x;
	hist.ents[n].y = y;
	hist.sz = n+1;

	int fd = open(histpath, O_RDWR|O_CREAT, 0600);
	if (fd < 0) {
		perror("open");
		exit(-1);
	}

	write(fd, &hist, sizeof(hist));
	close(fd);
}
