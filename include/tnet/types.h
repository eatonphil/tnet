#ifndef TNET_TYPES_H
#define TNET_TYPES_H

#include <stdint.h>

typedef struct __attribute__((__packed__)) {
  uint16_t sourcePort;
  uint16_t destPort;
  uint32_t sequenceNumber;
  uint32_t ackNumber;
  uint16_t dataOffset : 4;
  uint16_t reserved : 6;
  uint16_t urgFlag : 1;
  uint16_t ackFlag : 1;
  uint16_t pshFlag : 1;
  uint16_t rstFlag : 1;
  uint16_t synFlag : 1;
  uint16_t finFlag : 1;
  uint16_t windowSize;
  uint16_t checksum;
  uint16_t urgent;
  uint8_t options[40];
} TNET_TCPSegmentHeader;

typedef struct __attribute__((__packed__)) {
  uint16_t hardwareType;
  uint16_t protocolType;
  uint8_t hardwareAddressLength;
  uint8_t protocolAddressLength;
  uint16_t operation;
  uint8_t sourceHardwareAddress[6];
  uint16_t sourceProtocolAddress;
  uint8_t destHardwareAddress[6];
  uint16_t destProtocolAddress;
} TNET_ARPPacketHeader;

typedef struct __attribute__((__packed__)) {
  uint8_t version : 4;
  uint8_t length : 4;
  uint8_t typeOfService;
  uint16_t totalLength;
  uint16_t identification;
  uint16_t flags : 3;
  uint16_t fragmentOffset : 13;
  uint8_t timeToLive;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t sourceIPAddress;
  uint32_t destIPAddress;
  uint8_t options[12];
} TNET_IPv4PacketHeader;

const uint16_t TNET_ETHERNET_TYPE_IPv4 = 0x0800;
const uint16_t TNET_ETHERNET_TYPE_ARP = 0x0806;

typedef struct __attribute__((__packed__)) {
  uint8_t destMACAddress[6];
  uint8_t sourceMACAddress[6];
  uint16_t type;
  uint8_t payload[1500];
  uint32_t frameCheckSequence;
} TNET_EthernetFrame;

#endif
