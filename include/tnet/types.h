#ifndef TNET_TYPES_H
#define TNET_TYPES_H

#include <stdint.h>

const uint16_t TNET_ETHERNET_TYPE_IPv4 = 0x0800;
const uint16_t TNET_ETHERNET_TYPE_ARP = 0x0806;

const int TNET_ETHERNET_PAYLOAD_SIZE = 1500;

typedef struct __attribute__((__packed__)) {
  uint8_t destMACAddress[6];
  uint8_t sourceMACAddress[6];
  uint16_t type;
  uint8_t payload[TNET_ETHERNET_PAYLOAD_SIZE];
  uint32_t frameCheckSequence;
} TNET_EthernetFrame;

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

const int TNET_IP_PAYLOAD_SIZE =
    TNET_ETHERNET_PAYLOAD_SIZE - sizeof(TNET_IPv4PacketHeader);

typedef struct __attribute__((__packed__)) {
  uint16_t sourcePort;
  uint16_t destPort;
  uint32_t sequenceNumber;
  uint32_t ackNumber;
  uint16_t offsetAndFlags;
  uint16_t windowSize;
  uint16_t checksum;
  uint16_t urgent;
  uint8_t options[40];
} TNET_TCPSegmentHeader;

const int TNET_TCP_PAYLOAD_SIZE =
    TNET_IP_PAYLOAD_SIZE - sizeof(TNET_TCPSegmentHeader);

#endif
