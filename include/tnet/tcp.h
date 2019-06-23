#ifndef TNET_TCP_H
#define TNET_TCP_H

#include <net/if.h>

void TNET_tcpServe(int socket, char ifname[IFNAMSIZ]);

int TNET_tcpInit(int nConnections);

void TNET_tcpCleanup();

#endif
