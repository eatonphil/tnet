#ifndef TNET_ARP_H
#define TNET_ARP_H

#include <stdint.h>

#define DEBUG_ARP_PACKET(arpPacket)                                            \
  printf(                                                                      \
      "ARP PACKET\n"                                                           \
      "Hardware type: %#06x\n"                                                 \
      "Protocol type: %#06x\n"                                                 \
      "Hardware address length: %d\n"                                          \
      "Protocol address length: %d\n"                                          \
      "Operation: %d\n"                                                        \
      "Source hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n"               \
      "Source protocol address: %d.%d.%d.%d\n"                                 \
      "Dest hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n"                 \
      "Dest protocol address: %d.%d.%d.%d\n\n",                                \
      ntohs(arpPacket.hardwareType), ntohs(arpPacket.protocolType),            \
      arpPacket.hardwareAddressLength, arpPacket.protocolAddressLength,        \
      ntohs(arpPacket.operation), arpPacket.sourceHardwareAddress[0],          \
      arpPacket.sourceHardwareAddress[1], arpPacket.sourceHardwareAddress[2],  \
      arpPacket.sourceHardwareAddress[3], arpPacket.sourceHardwareAddress[4],  \
      arpPacket.sourceHardwareAddress[5],                                      \
      arpPacket.sourceProtocolAddress & 0xff,                                  \
      (arpPacket.sourceProtocolAddress >> 8) & 0xff,                           \
      (arpPacket.sourceProtocolAddress >> 16) & 0xff,                          \
      (arpPacket.sourceProtocolAddress >> 24) & 0xff,                          \
      arpPacket.destHardwareAddress[0], arpPacket.destHardwareAddress[1],      \
      arpPacket.destHardwareAddress[2], arpPacket.destHardwareAddress[3],      \
      arpPacket.destHardwareAddress[4], arpPacket.destHardwareAddress[5],      \
      arpPacket.destProtocolAddress & 0xff,                                    \
      (arpPacket.destProtocolAddress >> 8) & 0xff,                             \
      (arpPacket.destProtocolAddress >> 16) & 0xff,                            \
      (arpPacket.destProtocolAddress >> 24) & 0xff)

#define TNET_ARP_PACKET_FROM_ETHERNET_FRAME(frame)                             \
  (*((TNET_ARPPacket *)(void *)&frame->payload))

typedef struct __attribute__((__packed__)) {
  uint16_t hardwareType;
  uint16_t protocolType;
  uint8_t hardwareAddressLength;
  uint8_t protocolAddressLength;
  uint16_t operation;
  uint8_t sourceHardwareAddress[6];
  uint32_t sourceProtocolAddress;
  uint8_t destHardwareAddress[6];
  uint32_t destProtocolAddress;
} TNET_ARPPacket;

#endif
