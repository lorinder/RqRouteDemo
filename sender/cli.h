#ifndef CLI_H
#define CLI_H

#include "udp.h"

int cli_init();
void cli_prompt();
void cli_commands(double* pOverhead, udp* pUdp);

#endif /* CLI_H */
