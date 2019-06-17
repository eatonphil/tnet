#ifndef TNET_TCP_H
#define TNET_TCP_H

void TNET_tcpServe(int socket);

int TNET_tcpInit(int nConnections);

void TNET_tcpCleanup();

#endif
