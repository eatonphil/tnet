#ifndef TNET_NETWORK_H
#define TNET_NETWORK_H

#include <net/if.h>

#include <tnet/tapdevice.h>
#include <tnet/tcp.h>

typedef struct {
  // Private
  TNET_tcpipv4_Connection *connections;
  int count;
  int filled;
  TNET_tapdevice_Tapdevice *tapdevice;

  // Public
  void (*Serve)(TNET_network_Network *network);
  void (*Cleanup)(TNET_network_Network *network);
} TNET_network_Network;

int TNET_network_Network_Init(TNET_network_Network *,
                              TNET_tapdevice_Tapdevice *, int);

#endif
