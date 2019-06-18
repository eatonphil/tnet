#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "tnet/tcp.h"
#include "tnet/types.h"

#define DEBUG(str) printf("TNET: %s\n", str)

#define DEBUG_SEGMENT(frame)                                                   \
  {                                                                            \
    auto segment = TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);                \
    printf("Seq: %d, ack: %d, fin: %s, urg: %s, "                              \
           "psh: %s, rst: %s, syn: %s, ack: %s\n",                             \
           segment.sequenceNumber, segment.ackNumber,                          \
           segment.finFlag ? "true" : "false",                                 \
           segment.urgFlag ? "true" : "false",                                 \
           segment.pshFlag ? "true" : "false",                                 \
           segment.rstFlag ? "true" : "false",                                 \
           segment.synFlag ? "true" : "false",                                 \
           segment.ackFlag ? "true" : "false");                                \
  }

// http://home.thep.lu.se/~bjorn/crc/crc32_simple.c
uint32_t crc32_for_byte(uint32_t r) {
  for (int j = 0; j < 8; ++j)
    r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t *crc) {
  static uint32_t table[0x100];
  if (!*table)
    for (size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for (size_t i = 0; i < n_bytes; ++i)
    *crc = table[(uint8_t)*crc ^ ((uint8_t *)data)[i]] ^ *crc >> 8;
}

uint16_t ip_checksum(void *vdata, size_t length) {
  // Cast the data pointer to one that can be indexed.
  char *data = (char *)vdata;

  // Initialise the accumulator.
  uint32_t acc = 0xffff;

  // Handle complete 16-bit blocks.
  for (size_t i = 0; i + 1 < length; i += 2) {
    uint16_t word;
    memcpy(&word, data + i, 2);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Handle any partial block at the end of the data.
  if (length & 1) {
    uint16_t word = 0;
    memcpy(&word, data + length - 1, 1);
    acc += ntohs(word);
    if (acc > 0xffff) {
      acc -= 0xffff;
    }
  }

  // Return the checksum in network byte order.
  return htons(~acc);
}

uint16_t tcp_checksum(uint8_t *data, uint16_t len) {
  uint64_t sum = 0;
  uint32_t *p = (uint32_t *)data;
  uint16_t i = 0;
  while (len >= 4) {
    sum = sum + p[i++];
    len -= 4;
  }
  if (len >= 2) {
    sum = sum + ((uint16_t *)data)[i * 4];
    len -= 2;
  }
  if (len == 1) {
    sum += data[len - 1];
  }

  // Fold sum into 16-bit word.
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  return ntohs((uint16_t)~sum);
}

#define TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame)                            \
  (*((TNET_IPv4PacketHeader *)(void *)&frame->payload))

#define TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame)                            \
  (*(TNET_TCPSegmentHeader *)(void *)&frame                                    \
        ->payload[TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame).length])

typedef enum {
  TNET_TCP_STATE_CLOSED,
  TNET_TCP_STATE_LISTEN,
  TNET_TCP_STATE_SYN_RECEIVED,
  TNET_TCP_STATE_ESTABLISHED,
} TNET_TCPState;

typedef struct {
  int socket;
  TNET_TCPState state;
  uint16_t sequenceNumber;
  uint32_t sourceIPAddress;
  uint16_t sourcePort;
  uint32_t destIPAddress;
  uint16_t destPort;
} TNET_TCPIPv4Connection;

struct {
  TNET_TCPIPv4Connection *connections;
  int count;
  int filled;
} TNET_TCPIPv4Connections;

TNET_TCPIPv4Connection *TNET_tcpGetConnection(TNET_EthernetFrame *frame) {
  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPIPv4Connection conn;
  int i = 0;
  for (; i < TNET_TCPIPv4Connections.filled; i++) {
    conn = TNET_TCPIPv4Connections.connections[i];
    if (conn.sourceIPAddress == packetHeader.sourceIPAddress &&
        conn.sourcePort == segmentHeader.sourcePort &&
        conn.destIPAddress == packetHeader.destIPAddress &&
        conn.destPort == segmentHeader.destPort) {
      return TNET_TCPIPv4Connections.connections + i;
    }
  }

  if (i != TNET_TCPIPv4Connections.count - 1) {
    return NULL;
  }

  return NULL;
}

TNET_TCPIPv4Connection *TNET_tcpNewConnection(TNET_EthernetFrame *frame,
                                              int socket,
                                              uint16_t initialSequenceNumber) {
  int filled = TNET_TCPIPv4Connections.filled;
  int count = TNET_TCPIPv4Connections.count;
  if (filled == count) {
    return NULL;
  }

  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPIPv4Connection conn = {
      .socket = socket,
      .state = TNET_TCP_STATE_LISTEN,
      .sequenceNumber = initialSequenceNumber,
      .sourceIPAddress = packetHeader.sourceIPAddress,
      .sourcePort = segmentHeader.sourcePort,
      .destIPAddress = packetHeader.destIPAddress,
      .destPort = segmentHeader.destPort,
  };

  TNET_TCPIPv4Connections.connections[TNET_TCPIPv4Connections.filled++] = conn;

  return TNET_TCPIPv4Connections.connections + filled + 1;
}

void TNET_tcpWrite(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame,
                   TNET_TCPSegmentHeader *outSegmentHeader, uint8_t *msg,
                   int msgLen) {
  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);

  TNET_EthernetFrame outFrame;
  TNET_IPv4PacketHeader outPacketHeader;

  // Set up IP packet header
  outPacketHeader.length = htonl(32);
  outPacketHeader.totalLength = htonl(32 + 60 + msgLen);
  outPacketHeader.sourceIPAddress = packetHeader.destIPAddress;
  outPacketHeader.destIPAddress = packetHeader.sourceIPAddress;
  outPacketHeader.protocol = 0x2;
  outPacketHeader.checksum =
      ip_checksum(&outPacketHeader, outPacketHeader.totalLength);

  // Set up Ethernet frame
  memcpy(outFrame.destMACAddress, frame->sourceMACAddress, 6);
  memcpy(outFrame.sourceMACAddress, frame->destMACAddress, 6);
  outFrame.length = htonl(1522);
  memcpy(outFrame.payload, &outPacketHeader, outPacketHeader.totalLength);
  memcpy(outFrame.payload + 32, &outSegmentHeader, 60 + msgLen);
  memcpy(outFrame.payload + 32 + 60, msg, msgLen);
  memset(outFrame.payload + 32 + 60 + msgLen, 0, 1500 - (32 + 60 + msgLen));
  uint32_t crc;
  crc32((uint8_t *)&outFrame, 1518, &crc);
  outFrame.frameCheckSequence = htonl(crc);

  write(conn->socket, (uint8_t *)&outFrame, 1522);
}

void TNET_tcpSynAck(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPSegmentHeader outSegmentHeader;

  uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.ackNumber = htonl(ntohl(segmentHeader.sequenceNumber) + 1);
  outSegmentHeader.sequenceNumber = htonl(conn->sequenceNumber);
  outSegmentHeader.ackFlag = 1;
  outSegmentHeader.synFlag = 1;
  outSegmentHeader.windowSize = segmentHeader.windowSize;
  outSegmentHeader.checksum =
      tcp_checksum((uint8_t *)&outSegmentHeader, 60 + sizeof(msg));
  outSegmentHeader.dataOffset = htonl(60);

  TNET_tcpWrite(conn, frame, &outSegmentHeader, msg, sizeof(msg));
}

/* void TNET_tcpAck(int connection, TNET_EthernetFrame *frame) { */
/*   TNET_TCPSegmentHeader segmentHeader = */
/*       TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame); */

/*   TNET_TCPSegmentHeader outSegmentHeader; */

/*   uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>"; */

/*   // Set up TCP segment header */
/*   outSegmentHeader.sourcePort = segmentHeader.destPort; */
/*   outSegmentHeader.destPort = segmentHeader.sourcePort; */
/*   outSegmentHeader.ackNumber = htonl(ntohl(segmentHeader.ackNumber) + 1); */
/*   outSegmentHeader.sequenceNumber = segmentHeader.sequenceNumber; */
/*   outSegmentHeader.ackFlag = 1; */
/*   outSegmentHeader.windowSize = segmentHeader.windowSize; */
/*   outSegmentHeader.checksum = */
/*       tcp_checksum((uint8_t *)&outSegmentHeader, 60 + sizeof(msg)); */
/*   outSegmentHeader.dataOffset = htonl(60); */

/*   TNET_tcpWrite(connection, frame, &outSegmentHeader, msg, sizeof(msg)); */
/* } */

void TNET_tcpSend(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPSegmentHeader outSegmentHeader;

  uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.sequenceNumber = htonl(conn->sequenceNumber++);
  outSegmentHeader.ackFlag = 1;
  outSegmentHeader.windowSize = segmentHeader.windowSize;
  outSegmentHeader.checksum =
      tcp_checksum((uint8_t *)&outSegmentHeader, 60 + sizeof(msg));
  outSegmentHeader.dataOffset = htonl(60);

  TNET_tcpWrite(conn, frame, &outSegmentHeader, msg, sizeof(msg));
}

void TNET_tcpServe(int socket) {
  uint8_t ethernetBytes[1522] = {0};
  int err;
  TNET_EthernetFrame *frame;
  TNET_TCPIPv4Connection *conn;

  DEBUG("Listening");

  while (1) {
    DEBUG("Reading bytes");

    err = read(socket, &ethernetBytes, 1522);
    if (err == -1) {
      return;
    }

    DEBUG("Read bytes");

    frame = (TNET_EthernetFrame *)&ethernetBytes;

    DEBUG_SEGMENT(frame);

    conn = TNET_tcpGetConnection(frame);
    if (conn == NULL) {
      conn = TNET_tcpNewConnection(frame, socket, rand());
      if (conn == NULL) {
        break;
      }
    }

    switch (conn->state) {
    case TNET_TCP_STATE_CLOSED:
      break;
    case TNET_TCP_STATE_LISTEN:
      DEBUG("Sending syn-ack");
      TNET_tcpSynAck(conn, frame);
      DEBUG("Sent syn-ack");
      conn->state = TNET_TCP_STATE_SYN_RECEIVED;
      break;
    case TNET_TCP_STATE_SYN_RECEIVED:
      DEBUG("Sending data");
      TNET_tcpSend(conn, frame);
      DEBUG("Sent data");
      conn->state = TNET_TCP_STATE_ESTABLISHED;
      break;
    case TNET_TCP_STATE_ESTABLISHED:
      // Ignore any incoming frames for now.
      break;
    }
  }
}

int TNET_tcpInit(int count) {
  TNET_TCPIPv4Connection *connections =
      (TNET_TCPIPv4Connection *)malloc(sizeof(TNET_TCPIPv4Connection) * count);

  if (connections == NULL) {
    return -1;
  }

  TNET_TCPIPv4Connections.connections = connections;
  TNET_TCPIPv4Connections.count = count;
  TNET_TCPIPv4Connections.filled = 0;

  srand(time(NULL));

  return 0;
}

void TNET_tcpCleanup() { free(TNET_TCPIPv4Connections.connections); }
