#ifndef TNET_TCP_H
#define TNET_TCP_H

#include <stdint.h>

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
} TNET_tcp_TCPSegmentHeader;

const int TNET_TCP_PAYLOAD_SIZE =
    TNET_IP_PAYLOAD_SIZE - sizeof(TNET_TCPSegmentHeader);

#define DEBUG_TCP_SEGMENT(frame)                                               \
  {                                                                            \
    auto segment = TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);                \
    printf("TCP SEGMENT\n"                                                     \
           "Source port: %d\n"                                                 \
           "Dest port: %d\n"                                                   \
           "Sequence: %d\n"                                                    \
           "Ack: %d\n"                                                         \
           "Offset: %d\n"                                                      \
           "Reserved: %d\n"                                                    \
           "Flag[cwr]: %s\n"                                                   \
           "Flag[ece]: %s\n"                                                   \
           "Flag[urg]: %s\n"                                                   \
           "Flag[ack]: %s\n"                                                   \
           "Flag[psh]: %s\n"                                                   \
           "Flag[rst]: %s\n"                                                   \
           "Flag[syn]: %s\n"                                                   \
           "Flag[fin]: %s\n"                                                   \
           "Window size: %d\n"                                                 \
           "Checksum: %d\n"                                                    \
           "Urgent: %d\n\n",                                                   \
           ntohs(segment.sourcePort), ntohs(segment.destPort),                 \
           ntohl(segment.sequenceNumber), ntohl(segment.ackNumber),            \
           TNET_TCP_SEGMENT_DATA_OFFSET(segment),                              \
           TNET_TCP_SEGMENT_RESERVED(segment),                                 \
           TNET_TCP_SEGMENT_CWR_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_ECE_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_URG_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_ACK_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_PSH_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_RST_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_SYN_FLAG(segment) ? "true" : "false",              \
           TNET_TCP_SEGMENT_FIN_FLAG(segment) ? "true" : "false",              \
           ntohs(segment.windowSize), ntohs(segment.checksum),                 \
           ntohs(segment.urgent));                                             \
  }

#define TNET_TCP_SEGMENT_DATA_OFFSET(segment)                                  \
  ((segment.offsetAndFlags >> 4) & 0b1111)
#define TNET_TCP_SEGMENT_SET_DATA_OFFSET(segment, offset)                      \
  segment.offsetAndFlags |= (offset << 4) & 0b1111
#define TNET_TCP_SEGMENT_RESERVED(segment) (segment.offsetAndFlags & 0b1111)
#define TNET_TCP_SEGMENT_SET_RESERVED(segment, reserved)                       \
  segment.offsetAndFlags |= reserved & 0b1111
#define TNET_TCP_SEGMENT_CWR_FLAG(segment)                                     \
  ((segment.offsetAndFlags << 15) & 0b1)
#define TNET_TCP_SEGMENT_SET_CWR_FLAG(segment, cwr)                            \
  segment.offsetAndFlags |= (cwr << 15) & 0b1
#define TNET_TCP_SEGMENT_ECE_FLAG(segment)                                     \
  ((segment.offsetAndFlags >> 14) & 0b1)
#define TNET_TCP_SEGMENT_SET_ECE_FLAG(segment, ece)                            \
  segment.offsetAndFlags |= (ece << 14) & 0b1
#define TNET_TCP_SEGMENT_URG_FLAG(segment)                                     \
  ((segment.offsetAndFlags >> 13) & 0b1)
#define TNET_TCP_SEGMENT_SET_URG_FLAG(segment, urg)                            \
  segment.offsetAndFlags |= (urg << 13) & 0b1
#define TNET_TCP_SEGMENT_ACK_FLAG(segment)                                     \
  ((segment.offsetAndFlags >> 12) & 0b1)
#define TNET_TCP_SEGMENT_SET_ACK_FLAG(segment, ack)                            \
  segment.offsetAndFlags |= (ack << 12) & 0b1
#define TNET_TCP_SEGMENT_PSH_FLAG(segment) (segment.offsetAndFlags >> 11) & 0b1
#define TNET_TCP_SEGMENT_SET_PSH_FLAG(segment, psh)                            \
  segment.offsetAndFlags |= (psh << 11) & 0b1
#define TNET_TCP_SEGMENT_RST_FLAG(segment)                                     \
  ((segment.offsetAndFlags >> 10) & 0b1)
#define TNET_TCP_SEGMENT_SET_RST_FLAG(segment, rst)                            \
  segment.offsetAndFlags |= (rst << 10) & 0b1
#define TNET_TCP_SEGMENT_SYN_FLAG(segment) ((segment.offsetAndFlags >> 9) & 0b1)
#define TNET_TCP_SEGMENT_SET_SYN_FLAG(segment, syn)                            \
  segment.offsetAndFlags |= (syn << 9) & 0b1
#define TNET_TCP_SEGMENT_FIN_FLAG(segment) ((segment.offsetAndFlags >> 8) & 0b1)
#define TNET_TCP_SEGMENT_SET_FLAG_FLAG(segment, flag)                          \
  segment.offsetAndFlags |= (flag << 8) & 0b1

#define TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame)                            \
  (*(TNET_TCPSegmentHeader *)(void *)&frame                                    \
        ->payload[TNET_IPv4_PACKET_LENGTH(                                     \
                      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame)) *           \
                  4])

#endif
