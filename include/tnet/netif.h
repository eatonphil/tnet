#ifndef TNET_TAP_H
#define TNET_TAP_H

#include <stdint.h>

#include <net/if.h>

typedef struct TNET_netif_Netif TNET_netif_Netif;
struct TNET_netif_Netif {
  // Private
  char ifname[IFNAMSIZ];
  int fd;
  uint32_t ipv4Address;

  // Public
  uint8_t Mac[6];
  int (*Read)(TNET_netif_Netif *netif, uint8_t *buffer, int bufferSize);
  int (*Write)(TNET_netif_Netif *netif, uint8_t *buffer, int bufferSize);
  void (*Cleanup)(TNET_netif_Netif *netif);
};

int TNET_netif_Netif_Init(TNET_netif_Netif *, uint32_t address,
                          uint32_t gateway);

#endif
