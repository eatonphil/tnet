#ifndef TNET_NETWORK_H
#define TNET_NETWORK_H

#include <net/if.h>

#include <tnet/ethernet.h>
#include <tnet/netif.h>
#include <tnet/tcpipv4.h>

typedef struct TNET_network_Network TNET_network_Network;
struct TNET_network_Network {
  // Private
  TNET_tcpipv4_TCPIPv4 *tcpipv4;

  // Public
  void (*Serve)(TNET_network_Network *network, TNET_netif_Netif *netif,
                TNET_ethernet_Frame *frame);
  void (*Cleanup)(TNET_network_Network *network);
};

int TNET_network_Network_Init(TNET_network_Network *network, int nConnections);

#endif
