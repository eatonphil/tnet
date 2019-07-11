#include "tnet/ethernet.h"

#include <stddef.h>
#include <string.h>

#include <arpa/inet.h>

#include "tnet/tcpipv4.h"
#include "tnet/util.h"

// Source: http://home.thep.lu.se/~bjorn/crc/crc32_simple.c
uint32_t TNET_ethernet_crc32ForByte(uint32_t r) {
  for (int j = 0; j < 8; ++j)
    r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

uint32_t TNET_ethernet_crc32(const void *data, size_t n_bytes) {
  uint32_t crc;
  static uint32_t table[0x100];
  if (!*table)
    for (size_t i = 0; i < 0x100; ++i)
      table[i] = TNET_ethernet_crc32ForByte(i);
  for (size_t i = 0; i < n_bytes; ++i)
    crc = table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
  return htonl(crc);
}

void TNET_ethernet_Send(TNET_netif_Netif *netif, TNET_ethernet_Frame *frame,
                        uint16_t type, uint8_t *msg, int msgLen) {
  TNET_ethernet_Frame outFrame = {0};
  memcpy(outFrame.destMACAddress, frame->sourceMACAddress, sizeof(netif->Mac));
  memcpy(outFrame.sourceMACAddress, netif->Mac, sizeof(netif->Mac));
  outFrame.type = htons(type);
  memcpy(outFrame.payload, msg, msgLen);

  // Calculate checksum
  int frameLength =
      sizeof(TNET_ethernet_Frame) - TNET_ETHERNET_PAYLOAD_SIZE + msgLen;
  uint32_t crc = TNET_ethernet_crc32((uint8_t *)&outFrame, frameLength);
  memcpy((uint8_t *)&outFrame.payload + msgLen, &crc, sizeof(crc));

  if (type == TNET_ETHERNET_TYPE_IPv4) {
    TNET_DEBUG("Sending TCP segment");
    DEBUG_TCP_SEGMENT((&outFrame));

    TNET_DEBUG("Sending IPv4 packet");
    DEBUG_IPv4_PACKET((&outFrame));
  }

  netif->Write(netif, (uint8_t *)&outFrame, frameLength);
}
