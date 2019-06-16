#include "tnet/tcp.h"

#include <stdlib.h>
#include <time.h>

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

enum TNET_TCPState {
  TNET_TCP_STATE_CLOSED,
  TNET_TCP_STATE_LISTEN,
  TNET_TCP_STATE_SYN_RECEIVED,
  TNET_TCP_STATE_ESTABLISHED,
}

TNET_TCPState state = TNET_TCP_STATE_CLOSED;

// TODO: Use connection tuple to get/set state.
#define TNET_TCP_STATE(packetHeader, segmentHeader) state
#define TNET_TCP_SET_STATE(packetHeader, segmentHeader, newState)              \
  state = newState

#define TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame)                            \
  ((TNET_IPv4PacketHeader)frame.payload)

#define TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame)                            \
  ((TNET_TCPSegmentHeader)                                                     \
       frame.length[((TNET_IPv4PacketHeader)frame.payload).length])

void TNET_tcpWrite(int *connection, TNET_EthernetFrame *frame,
                   TNET_TCPSegmentHeader *segmentHeader, uint8_t *msg,
                   int msgLen) {
  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(*frame);
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(*frame);

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
  memcpy(outFrame.destMACAddress, 6, frame.sourceMACAddress);
  memcpy(outFrame.sourceMACAddress, 6, frame.destMACAddress);
  outFrame.length = htonl(1522);
  memcpy(outFrame.payload, outPacketHeader.totalLength, outPacketHeader);
  memcpy(outFrame.payload + 32, 60 + msgLen, outSegmentHeader);
  memcpy(outFrame.payload + 32 + 60, msgLen, msg);
  memset(outFrame.payload + 32 + 60 + msgLen, 1500 - (32 + 60 + msgLen), 0);
  uint32_t crc;
  crc32(&outFrame, 1518, &crc);
  outFrame.frameCheckSequence = htonl(crc);

  write(connection, outFrame, 1522);
}

void TNET_tcpSynAck(int *connection, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader outSegmentHeader;

  uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.ackNumber = htonl(ntohl(segmentHeader.ackNumber) + 1);
  outSegmentHeader.sequenceNumber = htonl(rand());
  outSegmentHeader.ackFlag = 1;
  outSegmentHeader.synFlag = 1;
  outSegmentHeader.windowSize = segmentHeader.windowSize;
  outSegmentHeader.checksum = tcp_checksum(&outSegmentHeader, 60 + sizeof(msg));
  outSegmentHeader.dataOffset = htonl(60);

  TNET_tcpWrite(connection, frame, &outSegmentHeader, msg, sizeof(msg));
}

void TNET_tcpSend(int *connection, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader outSegmentHeader;

  uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.sequenceNumber =
      htonl(ntohl(segmentHeader.sequenceNumber) + 1);
  outSegmentHeader.ackFlag = 1;
  outSegmentHeader.windowSize = segmentHeader.windowSize;
  outSegmentHeader.checksum = tcp_checksum(&outSegmentHeader, 60 + sizeof(msg));
  outSegmentHeader.dataOffset = htonl(60);

  TNET_tcpWrite(connection, frame, &outSegmentHeader, msg, sizeof(msg));
}

void TNET_tcpServe(int *connection) {
  uint8_t ethernetBytes[1522] = {0};
  int err;
  TNET_EthernetFrame frame;
  TNET_IPv4PacketHeader packetHeader;
  TNET_TCPSegmentHeader segmentHeader;

  while (1) {
    err = read(connection, &ethernetBytes);
    if (err == -1) {
      return;
    }

    frame = (TNET_EthernetFrame)ethernetBytes;

    switch (TNET_TCP_STATE(frame)) {
    case TNET_TCP_STATE_LISTEN:
      TNET_tcpSynAck(connection, &frame);
      TNET_TCP_SET_STATE(frame, TNET_TCP_STATE_SYN_RECEIVED);
      break;
    case TNET_TCP_STATE_SYN_RECEIVED:
      TNET_tcpSend(connection, &frame);
      TNET_TCP_SET_STATE(frame, TNET_TCP_STATE_ESTABLISHED);
      break;
    case TNET_TCP_STATE_ESTABLISHED:
      // Ignore any incoming frames for now.
      break;
    }
  }
}

void TNET_tcpInit() {
  TNET_TCP_SET_STATE(TNET_TCP_STATE_LISTEN);
  srand(time(NULL));
}

void TNET_tcpCleanup() {}
