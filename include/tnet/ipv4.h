#ifndef TNET_IPv4_H
#define TNET_IPv4_H

#include <stdint.h>

#include "tnet/ethernet.h"

#define DEBUG_IPv4_PACKET(frame)                                               \
  {                                                                            \
    auto ipPacket = TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);               \
    printf("IPv4 PACKET\n"                                                     \
           "Version: %d\n"                                                     \
           "Header length: %d\n"                                               \
           "Type of service: %d\n"                                             \
           "Length: %u\n"                                                      \
           "Id: %u\n"                                                          \
           "Flags and fragment offset: %0x02\n"                                \
           "Time to live: %d\n"                                                \
           "Protocol: %d\n"                                                    \
           "Checksum: %u\n"                                                    \
           "Source address: %d.%d.%d.%d\n"                                     \
           "Dest address: %d.%d.%d.%d\n\n",                                    \
           TNET_IPv4_PACKET_VERSION(ipPacket),                                 \
           TNET_IPv4_PACKET_LENGTH(ipPacket), ipPacket.typeOfService,          \
           ntohs(ipPacket.totalLength), ntohs(ipPacket.identification),        \
           ntohs(ipPacket.flagsAndFragmentOffset), ipPacket.timeToLive,        \
           ipPacket.protocol, ntohs(ipPacket.checksum),                        \
           ipPacket.sourceIPAddress & 0xff,                                    \
           (ipPacket.sourceIPAddress >> 8) & 0xff,                             \
           (ipPacket.sourceIPAddress >> 16) & 0xff,                            \
           (ipPacket.sourceIPAddress >> 24) & 0xff,                            \
           ipPacket.destIPAddress & 0xff,                                      \
           (ipPacket.destIPAddress >> 8) & 0xff,                               \
           (ipPacket.destIPAddress >> 16) & 0xff,                              \
           (ipPacket.destIPAddress >> 24) & 0xff);                             \
  }

#define TNET_IPv4_PACKET_LENGTH(packet) (packet.versionAndLength & 0b1111)
#define TNET_IPv4_PACKET_SET_LENGTH(packet, length)                            \
  packet.versionAndLength |= length & 0b1111
#define TNET_IPv4_PACKET_VERSION(packet)                                       \
  ((packet.versionAndLength >> 4) & 0b1111)
#define TNET_IPv4_PACKET_SET_VERSION(packet, version)                          \
  packet.versionAndLength |= (version << 4) & 0b1111

#define TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame)                            \
  (*((TNET_IPv4PacketHeader *)(void *)&frame->payload))

const uint16_t TNET_IP_TYPE_TCP = 6;

typedef struct __attribute__((__packed__)) {
  uint8_t versionAndLength;
  uint8_t typeOfService;
  uint16_t totalLength;
  uint16_t identification;
  uint16_t flagsAndFragmentOffset;
  uint8_t timeToLive;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t sourceIPAddress;
  uint32_t destIPAddress;
  uint8_t options[12];
} TNET_IPv4PacketHeader;

#define TNET_IP_PAYLOAD_SIZE                                                   \
  (TNET_ETHERNET_PAYLOAD_SIZE - sizeof(TNET_IPv4PacketHeader))

#endif
