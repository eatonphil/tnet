#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "tnet/tcp.h"
#include "tnet/types.h"

#define DEBUG(str) printf("TNET: %s\n", str)

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

#define DEBUG_ARP_PACKET(arpPacket)                                            \
  printf(                                                                      \
      "ARP PACKET\n"                                                           \
      "Hardware type: %#06x\n"                                                 \
      "Protocol type: %#06x\n"                                                 \
      "Hardware address length: %d\n"                                          \
      "Protocol address length: %d\n"                                          \
      "Operation: %d\n"                                                        \
      "Source hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n"               \
      "Source protocol address: %d.%d.%d.%d\n"                                 \
      "Dest hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n"                 \
      "Dest protocol address: %d.%d.%d.%d\n\n",                                \
      ntohs(arpPacket.hardwareType), ntohs(arpPacket.protocolType),            \
      arpPacket.hardwareAddressLength, arpPacket.protocolAddressLength,        \
      ntohs(arpPacket.operation), arpPacket.sourceHardwareAddress[0],          \
      arpPacket.sourceHardwareAddress[1], arpPacket.sourceHardwareAddress[2],  \
      arpPacket.sourceHardwareAddress[3], arpPacket.sourceHardwareAddress[4],  \
      arpPacket.sourceHardwareAddress[5],                                      \
      arpPacket.sourceProtocolAddress & 0xff,                                  \
      (arpPacket.sourceProtocolAddress >> 8) & 0xff,                           \
      (arpPacket.sourceProtocolAddress >> 16) & 0xff,                          \
      (arpPacket.sourceProtocolAddress >> 24) & 0xff,                          \
      arpPacket.destHardwareAddress[0], arpPacket.destHardwareAddress[1],      \
      arpPacket.destHardwareAddress[2], arpPacket.destHardwareAddress[3],      \
      arpPacket.destHardwareAddress[4], arpPacket.destHardwareAddress[5],      \
      arpPacket.destProtocolAddress & 0xff,                                    \
      (arpPacket.destProtocolAddress >> 8) & 0xff,                             \
      (arpPacket.destProtocolAddress >> 16) & 0xff,                            \
      (arpPacket.destProtocolAddress >> 24) & 0xff)

// http://home.thep.lu.se/~bjorn/crc/crc32_simple.c
uint32_t crc32_for_byte(uint32_t r) {
  for (int j = 0; j < 8; ++j)
    r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
  return r ^ (uint32_t)0xFF000000L;
}

uint32_t crc32(const void *data, size_t n_bytes) {
  uint32_t crc;
  static uint32_t table[0x100];
  if (!*table)
    for (size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  for (size_t i = 0; i < n_bytes; ++i)
    crc = table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
  return crc;
}

uint16_t tcpip_checksum(uint8_t *data, size_t length) {
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

#define TNET_ARP_PACKET_FROM_ETHERNET_FRAME(frame)                             \
  (*((TNET_ARPPacket *)(void *)&frame->payload))

#define TNET_IPv4_PACKET_LENGTH(packet) (packet.versionAndLength & 0b1111)
#define TNET_IPv4_PACKET_SET_LENGTH(packet, length)                            \
  packet.versionAndLength |= length & 0b1111
#define TNET_IPv4_PACKET_VERSION(packet)                                       \
  ((packet.versionAndLength >> 4) & 0b1111)
#define TNET_IPv4_PACKET_SET_VERSION(packet, version)                          \
  packet.versionAndLength |= (version << 4) & 0b1111

#define TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame)                            \
  (*((TNET_IPv4PacketHeader *)(void *)&frame->payload))

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

typedef enum {
  TNET_TCP_STATE_CLOSED,
  TNET_TCP_STATE_LISTEN,
  TNET_TCP_STATE_SYN_RECEIVED,
  TNET_TCP_STATE_ESTABLISHED,
} TNET_TCPState;

typedef struct {
  int sock;
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
  uint8_t mac[6];
  uint32_t ip;
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
                                              int sock,
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
      .sock = sock,
      .state = TNET_TCP_STATE_LISTEN,
      .sequenceNumber = initialSequenceNumber,
      .sourceIPAddress = packetHeader.sourceIPAddress,
      .sourcePort = segmentHeader.sourcePort,
      .destIPAddress = packetHeader.destIPAddress,
      .destPort = segmentHeader.destPort,
  };

  // Store new connection
  TNET_TCPIPv4Connections.connections[TNET_TCPIPv4Connections.filled++] = conn;

  return TNET_TCPIPv4Connections.connections + filled + 1;
}

void TNET_ethSend(int sock, TNET_EthernetFrame *frame, uint16_t type,
                  uint8_t *msg, int msgLen) {
  TNET_EthernetFrame outFrame = {0};
  memcpy(outFrame.destMACAddress, frame->sourceMACAddress,
         sizeof(TNET_TCPIPv4Connections.mac));
  memcpy(outFrame.sourceMACAddress, TNET_TCPIPv4Connections.mac,
         sizeof(TNET_TCPIPv4Connections.mac));
  outFrame.type = htons(type);
  memcpy(outFrame.payload, msg, msgLen);

  // Calculate checksum
  int frameLength =
      sizeof(TNET_EthernetFrame) - TNET_ETHERNET_PAYLOAD_SIZE + msgLen;
  uint32_t crc = htonl(crc32((uint8_t *)&outFrame, frameLength));
  memcpy((uint8_t *)&outFrame.payload + msgLen, &crc, sizeof(crc));

  if (type == TNET_ETHERNET_TYPE_IPv4) {
    DEBUG("Sending TCP segment");
    DEBUG_TCP_SEGMENT((&outFrame));

    DEBUG("Sending IPv4 packet");
    DEBUG_IPv4_PACKET((&outFrame));
  }

  write(sock, (uint8_t *)&outFrame, frameLength);
}

void TNET_ipv4Send(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame,
                   uint8_t *msg, int msgLen) {
  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);

  TNET_IPv4PacketHeader outPacketHeader = {0};
  uint8_t payload[sizeof(outPacketHeader) + TNET_IP_PAYLOAD_SIZE] = {0};

  TNET_IPv4_PACKET_SET_VERSION(outPacketHeader, 4);
  // Expressed in 32-bit words
  TNET_IPv4_PACKET_SET_LENGTH(outPacketHeader, 8);
  outPacketHeader.totalLength = htons(32 + msgLen);
  outPacketHeader.sourceIPAddress = packetHeader.destIPAddress;
  outPacketHeader.destIPAddress = packetHeader.sourceIPAddress;
  outPacketHeader.protocol = 0x2;
  // Expressed in 32-bit words
  int packetOffset = TNET_IPv4_PACKET_LENGTH(outPacketHeader) * 4;
  outPacketHeader.checksum =
      tcpip_checksum((uint8_t *)&outPacketHeader, packetOffset);
  memcpy(payload + packetOffset, msg, msgLen);

  int packetSize = sizeof(TNET_IPv4PacketHeader) + msgLen;
  TNET_ethSend(conn->sock, frame, TNET_ETHERNET_TYPE_IPv4, payload, packetSize);
}

void TNET_tcpSynAck(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPSegmentHeader outSegmentHeader = {0};
  uint8_t payload[sizeof(outSegmentHeader) + TNET_TCP_PAYLOAD_SIZE] = {0};

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.ackNumber = htonl(ntohl(segmentHeader.sequenceNumber) + 1);
  outSegmentHeader.sequenceNumber = htonl(conn->sequenceNumber);
  TNET_TCP_SEGMENT_SET_ACK_FLAG(outSegmentHeader, 1);
  TNET_TCP_SEGMENT_SET_SYN_FLAG(outSegmentHeader, 1);
  outSegmentHeader.windowSize = segmentHeader.windowSize;

  int segmentHeaderSize = 20; // Ignore options
  // Expressed in terms of 32-bit words
  TNET_TCP_SEGMENT_SET_DATA_OFFSET(outSegmentHeader, segmentHeaderSize / 4);
  outSegmentHeader.checksum = tcpip_checksum(payload, segmentHeaderSize);

  memcpy(payload, &outSegmentHeader, segmentHeaderSize);

  TNET_ipv4Send(conn, frame, payload, segmentHeaderSize);
}

/* void TNET_tcpAck(int connection, TNET_EthernetFrame *frame) { */
/*   TNET_TCPSegmentHeader segmentHeader = */
/*       TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame); */

/*   TNET_TCPSegmentHeader outSegmentHeader; */

/*   uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";
 */

/*   // Set up TCP segment header */
/*   outSegmentHeader.sourcePort = segmentHeader.destPort; */
/*   outSegmentHeader.destPort = segmentHeader.sourcePort; */
/*   outSegmentHeader.ackNumber =
 * htonl(ntohl(segmentHeader.ackNumber) + 1); */
/*   outSegmentHeader.sequenceNumber = segmentHeader.sequenceNumber;
 */
/*   outSegmentHeader.ackFlag = 1; */
/*   outSegmentHeader.windowSize = segmentHeader.windowSize; */
/*   outSegmentHeader.checksum = */
/*       tcp_checksum((uint8_t *)&outSegmentHeader, 60 +
 * sizeof(msg)); */
/*   outSegmentHeader.dataOffset = htonl(60); */

/*   TNET_tcpWrite(connection, frame, &outSegmentHeader, msg,
 * sizeof(msg)); */
/* } */

void TNET_tcpSend(TNET_TCPIPv4Connection *conn, TNET_EthernetFrame *frame) {
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPSegmentHeader outSegmentHeader = {0};
  uint8_t payload[sizeof(outSegmentHeader) + TNET_TCP_PAYLOAD_SIZE] = {0};
  uint8_t msg[] = "HTTP/1.1 200 OK\r\n\r\n<h1>Hello world!</h1>";

  // Set up TCP segment header
  outSegmentHeader.sourcePort = segmentHeader.destPort;
  outSegmentHeader.destPort = segmentHeader.sourcePort;
  outSegmentHeader.sequenceNumber = htonl(conn->sequenceNumber++);
  TNET_TCP_SEGMENT_SET_ACK_FLAG(outSegmentHeader, 1);
  outSegmentHeader.windowSize = segmentHeader.windowSize;

  int segmentHeaderSize = 20; // Ignore options
  // Expressed in terms of 32-bit words
  TNET_TCP_SEGMENT_SET_DATA_OFFSET(outSegmentHeader, segmentHeaderSize / 4);

  memcpy(payload, &outSegmentHeader, segmentHeaderSize);
  memcpy(payload + segmentHeaderSize, msg, sizeof(msg));

  int segmentSize = segmentHeaderSize + sizeof(msg);
  outSegmentHeader.checksum = tcpip_checksum(payload, segmentSize);
  // Recopy once checksum has been calculated.
  memcpy(payload, &outSegmentHeader, segmentHeaderSize);

  TNET_ipv4Send(conn, frame, payload, segmentSize);
}

void TNET_arpRespond(int sock, TNET_EthernetFrame *frame) {
  TNET_ARPPacket packet = TNET_ARP_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_ARPPacket outPacket = {0};

  if (ntohs(packet.protocolType) != TNET_ETHERNET_TYPE_IPv4) {
    DEBUG("Dropping non-ipv4 arp request");
    return;
  }

  // Fill out rest
  outPacket.hardwareType = packet.hardwareType;
  outPacket.protocolType = packet.protocolType;
  outPacket.hardwareAddressLength = packet.hardwareAddressLength;
  outPacket.protocolAddressLength = packet.protocolAddressLength;
  outPacket.operation = htons(2); // Response
  memcpy(outPacket.sourceHardwareAddress, TNET_TCPIPv4Connections.mac,
         sizeof(outPacket.sourceHardwareAddress));
  outPacket.sourceProtocolAddress = packet.destProtocolAddress;
  memcpy(outPacket.destHardwareAddress, packet.sourceHardwareAddress, 6);
  outPacket.destProtocolAddress = packet.sourceProtocolAddress;

  TNET_ethSend(sock, frame, TNET_ETHERNET_TYPE_ARP, (uint8_t *)&outPacket,
               sizeof(outPacket));
}

void TNET_tcpServe(int sock) {
  uint8_t ethernetBytes[sizeof(TNET_EthernetFrame)] = {0};
  int err;
  TNET_EthernetFrame *frame;
  TNET_TCPIPv4Connection *conn;

  DEBUG("Listening");

  while (1) {
    err = read(sock, &ethernetBytes, sizeof(ethernetBytes));
    if (err == -1) {
      return;
    }

    frame = (TNET_EthernetFrame *)&ethernetBytes;

    printf("TNET: Received frame of type %#06x\n", ntohs(frame->type));

    if (frame->type == htons(TNET_ETHERNET_TYPE_ARP)) {
      TNET_ARPPacket packet = TNET_ARP_PACKET_FROM_ETHERNET_FRAME(frame);
      DEBUG_ARP_PACKET(packet);
      TNET_arpRespond(sock, frame);
      continue;
    }

    if (frame->type != htons(TNET_ETHERNET_TYPE_IPv4)) {
      DEBUG("Dropping frame");
      continue;
    }

    auto ipPacket = TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
    printf("TNET: Received packet version %d, protocol %d\n",
           TNET_IPv4_PACKET_VERSION(ipPacket), ipPacket.protocol);

    if (TNET_IPv4_PACKET_VERSION(ipPacket) != 4 ||
        ipPacket.protocol != TNET_IP_TYPE_TCP) {
      DEBUG("Dropping packet");
      continue;
    }

    DEBUG_IPv4_PACKET(frame);
    DEBUG_TCP_SEGMENT(frame);

    conn = TNET_tcpGetConnection(frame);
    if (conn == NULL) {
      conn = TNET_tcpNewConnection(frame, sock, rand());
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

int TNET_tcpInit(int count, char ifname[IFNAMSIZ]) {
  TNET_TCPIPv4Connection *connections =
      (TNET_TCPIPv4Connection *)malloc(sizeof(TNET_TCPIPv4Connection) * count);

  if (connections == NULL) {
    return -1;
  }

  TNET_TCPIPv4Connections.connections = connections;
  TNET_TCPIPv4Connections.count = count;
  TNET_TCPIPv4Connections.filled = 0;

  struct ifreq ifr = {0};
  strcpy(ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;

  // Fill out MAC address
  int querySock = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl(querySock, SIOCGIFHWADDR, &ifr) < 0) {
    DEBUG("Bad ioctl request for hardware address");
    return -1;
  }

  memcpy(TNET_TCPIPv4Connections.mac, ifr.ifr_hwaddr.sa_data,
         sizeof(TNET_TCPIPv4Connections.mac));

  // Fill out IP address
  struct ifreq ifr2 = {0};
  strcpy(ifr2.ifr_name, ifname);
  struct sockaddr_in sai = {0};
  sai.sin_family = AF_INET;
  sai.sin_port = 0;
  sai.sin_addr.s_addr = inet_addr("192.168.2.3");
  ifr2.ifr_addr = *((sockaddr *)&sai);

  if (ioctl(querySock, SIOCSIFADDR, &ifr2) < 0) {
    DEBUG("Bad ioctl request to set protocol address");
    return 0;
  }

  TNET_TCPIPv4Connections.ip = sai.sin_addr.s_addr;

  // Set gateway
  struct rtentry route = {0};
  route.rt_flags = RTF_UP | RTF_GATEWAY;
  route.rt_dev = ifname;
  struct sockaddr_in *addr = (struct sockaddr_in *)&route.rt_gateway;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr("192.168.2.1");
  addr = (struct sockaddr_in *)&route.rt_dst;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = INADDR_ANY;
  addr = (struct sockaddr_in *)&route.rt_genmask;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = INADDR_ANY;

  if (ioctl(querySock, SIOCADDRT, &route) < 0) {
    DEBUG("Bad ioctl request to set gateway");
    return 1;
  }

  srand(time(NULL));

  return 0;
}

void TNET_tcpCleanup() { free(TNET_TCPIPv4Connections.connections); }
