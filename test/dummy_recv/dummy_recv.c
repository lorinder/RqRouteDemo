#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char** argv)
{
	/* Create socket */
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("socket()");
		return 1;
	}

	/* bind socket */
	struct sockaddr_in A;
	memset(&A, 0, sizeof(A));
	A.sin_family = AF_INET;
	A.sin_addr.s_addr = INADDR_ANY;
	A.sin_port = htons(6910);
	if (bind(sock_fd, (const struct sockaddr*)&A, sizeof(A)) < 0) {
		perror("bind()");
		return 1;
	}

	char buf[2048];
	ssize_t sz;
	do {
		sz = recvfrom(sock_fd, buf, sizeof(buf), 0, NULL, NULL);
		if (sz <= 0)
			break;
		putc('.', stdout);
		fflush(stdout);
	} while (1);

	if (sz < 0) {
		perror("recvfrom()");
		return 1;
	}

	return 0;
}
