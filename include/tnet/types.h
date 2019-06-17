#ifndef TNET_TYPES_H
#define TNET_TYPES_H

#include <stdint.h>

typedef struct __attribute__((__packed__)) {
  uint16_t sourcePort;
  uint16_t destPort;
  uint32_t sequenceNumber;
  uint32_t ackNumber;
  uint16_t dataOffset : 4;
  uint16_t reserved : 3;
  uint16_t nsFlag : 1;
  uint16_t cwrFlag : 1;
  uint16_t eceFlag : 1;
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
  uint8_t version : 4;
  uint8_t length : 4;
  uint8_t dscp : 6;
  uint8_t ecn : 2;
  uint16_t totalLength;
  uint16_t identification;
  uint16_t flags : 3;
  uint16_t fragmentOffset : 13;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t sourceIPAddress;
  uint32_t destIPAddress;
  uint8_t options[12];
} TNET_IPv4PacketHeader;

typedef struct __attribute__((__packed__)) {
  uint8_t destMACAddress[6];
  uint8_t sourceMACAddress[6];
  uint32_t tag;
  uint16_t length;
  uint8_t payload[1500];
  uint32_t frameCheckSequence;
} TNET_EthernetFrame;

#endif
