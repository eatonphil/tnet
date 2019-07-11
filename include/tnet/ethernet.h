#ifndef TNET_ETHERNET_H
#define TNET_ETHERNET_H

#include <stdint.h>

#include "tnet/netif.h"

const uint16_t TNET_ETHERNET_TYPE_IPv4 = 0x0800;
const uint16_t TNET_ETHERNET_TYPE_ARP = 0x0806;

#define TNET_ETHERNET_PAYLOAD_SIZE 1500

typedef struct __attribute__((__packed__)) {
  uint8_t destMACAddress[6];
  uint8_t sourceMACAddress[6];
  uint16_t type;
  uint8_t payload[TNET_ETHERNET_PAYLOAD_SIZE];
  uint32_t frameCheckSequence;
} TNET_ethernet_Frame;

void TNET_ethernet_Send(TNET_netif_Netif *netif, TNET_ethernet_Frame *frame,
                        uint16_t type, uint8_t *msg, int msgLen);

#endif
