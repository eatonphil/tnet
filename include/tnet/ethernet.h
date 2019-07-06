#ifndef TNET_ETHERNET_H
#define TNET_ETHERNET_H

#include <stdint.h>

// Source: http://home.thep.lu.se/~bjorn/crc/crc32_simple.c
static uint32_t crc32_for_byte(uint32_t r) {
  for (int j = 0; j < 8; ++j)
    r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

static uint32_t crc32(const void *data, size_t n_bytes) {
  uint32_t crc;
  static uint32_t table[0x100];
  if (!*table)
    for (size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for (size_t i = 0; i < n_bytes; ++i)
    crc = table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
  return crc;
}

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

#endif
