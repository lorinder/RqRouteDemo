#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include "fd_ops.h"

#include "cli.h"

int cli_init()
{
	return make_nonblock(0);
}

void cli_prompt()
{
	printf("sender> ");
	fflush(stdout);
}

static bool get_next_line(char* dest, size_t dest_sz)
{
	/* We have the convention that buf[n_bytes_in_buf] = '\0' */
	static char buf[256] = { '\0' };
	static int n_bytes_in_buf = 0;

	char* p_search = buf;
	char* p;
	while ((p = strchr(p_search, '\n')) == NULL) {
		/* Read */
		const ssize_t
		  nbytes = read(0, &buf[n_bytes_in_buf], sizeof(buf) - n_bytes_in_buf - 1);
		if (nbytes == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return false;
			perror("read()");
			return false;
		}

		/* Check EOF */
		if (nbytes == 0)
			exit(0); // XXX: hack...

		/* Update pointers and counters */
		p_search = &buf[n_bytes_in_buf];
		n_bytes_in_buf += nbytes;
		buf[n_bytes_in_buf] = '\0';

		/* Check for overflow */
		if (n_bytes_in_buf == sizeof(buf) - 1) {
			fprintf(stderr, "Error: %s:%d:  Line longer than "
			  "supported, discarding.\n", __FILE__,
			  __LINE__);
			p_search = buf;
			n_bytes_in_buf = 0;
			*buf = '\0';
		}
	}

	/* Found '\n' */
	*p = '\0';
	if (dest_sz > p - buf) {
		strcpy(dest, buf);
	} else {
		memcpy(dest, buf, dest_sz - 1);
		dest[dest_sz - 1] = '\0';
	}
	size_t delta = (p + 1) - buf;
	n_bytes_in_buf -= delta;
	memmove(buf, p + 1, n_bytes_in_buf + 1);

	return true;
}

static void exec_cli_command(const char* cmd,
			const char* args,
			double* pOverhead,
			udp* pUdp)
{
	/* Execute command */
	if (strcmp(cmd, "overhead") == 0) {
		if (args) {
			sscanf(args, "%lg", pOverhead);
		}
		printf("overhead %g\n", *pOverhead);
	} else if (strcmp(cmd, "drop-prob") == 0) {
		double drop_prob;
		udp_params up;
		udp_get_params(pUdp, &up);
		if (args && sscanf(args, "%lg", &drop_prob) == 1) {
			up.drop_prob = drop_prob;
			udp_set_params(pUdp, &up);
		}
		printf("drop-prob %g\n", up.drop_prob);
	} else if (strcmp(cmd, "throttle-speed") == 0) {
		double throttle_speed;
		udp_params up;
		udp_get_params(pUdp, &up);
		if (args && sscanf(args, "%lg", &throttle_speed) == 1) {
			up.throttle_speed = throttle_speed;
			udp_set_params(pUdp, &up);
		}
		printf("throttle-speed %g\n", up.throttle_speed);
	} else if (strcmp(cmd, "help") == 0) {
		puts(	"Available commands:\n"
			"\n"
			"  overhead [#]       get/set the code overhead\n"
			"  drop-prob [#]      get/set package drop probability\n"
			"  throttle-speed [#] get/set transfer speed [Mbps]\n"
			"\n"
			"  help               display this help\n"
			"  exit               leave the program\n"
		);
	} else if (strcmp(cmd, "exit") == 0) {
		exit(0);
	} else {
		printf("Unknown command `%s'.\n", cmd);
		puts("Type `help' for a list of commands.");
	}
}

void cli_commands(double* pOverhead, udp* pUdp)
{
	char buf[80];
	while (get_next_line(buf, sizeof(buf))) {
		/* Split params from command */
		char* p = strchr(buf, ' ');
		if (p) {
			*p++ = '\0';
		}

		/* Execute the command */
		exec_cli_command(buf, p, pOverhead, pUdp);
	}
}
