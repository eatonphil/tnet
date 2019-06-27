#ifndef TNET_TCP_H
#define TNET_TCP_H

#include <net/if.h>

void TNET_tcpServe(int socket);

int TNET_tcpInit(int nConnections, char ifname[IFNAMSIZ]);

void TNET_tcpCleanup();

#endif
