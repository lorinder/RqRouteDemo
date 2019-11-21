#include <stdio.h>
#include <fcntl.h>

#include "fd_ops.h"

int make_nonblock(int fd)
{
	int mode = fcntl(fd, F_GETFL);
	mode |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, mode) == -1) {
		perror("fcntl(0, F_SETFL, ...)");
		return -1;
	}
	return 0;
}
