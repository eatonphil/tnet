#ifndef TNET_TCPIPv4_H
#define TNET_TCPIPv4_H

#include <stdint.h>

typedef struct {
  TNET_tcp_State state;
  uint16_t sequenceNumber;
  uint32_t sourceIPAddress;
  uint16_t sourcePort;
  uint32_t destIPAddress;
  uint16_t destPort;
} TNET_tcpipv4_Connection;

#endif;
