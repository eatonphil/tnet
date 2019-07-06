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

// Source: https://blogs.igalia.com/dpino/2018/06/14/fast-checksum-computation/
uint16_t checksum(uint8_t *data, uint16_t len) {
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

typedef enum {
  TNET_TCP_STATE_CLOSED,
  TNET_TCP_STATE_LISTEN,
  TNET_TCP_STATE_SYN_RECEIVED,
  TNET_TCP_STATE_ESTABLISHED,
} TNET_TCPState;

TNET_TCPIPv4Connection *TNET_tcpGetConnection(TNET_EthernetFrame *frame) {
  TNET_IPv4PacketHeader packetHeader =
      TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_TCPSegmentHeader segmentHeader =
      TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_TCPIPv4Connection conn;
  int i = 0;
  for (; i < network->filled; i++) {
    conn = network->connections[i];
    if (conn.sourceIPAddress == packetHeader.sourceIPAddress &&
        conn.sourcePort == segmentHeader.sourcePort &&
        conn.destIPAddress == packetHeader.destIPAddress &&
        conn.destPort == segmentHeader.destPort) {
      return network->connections + i;
    }
  }

  if (i != network->count - 1) {
    return NULL;
  }

  return NULL;
}

TNET_TCPIPv4Connection *TNET_tcpNewConnection(TNET_EthernetFrame *frame,
                                              int sock,
                                              uint16_t initialSequenceNumber) {
  int filled = network->filled;
  int count = network->count;
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
  network->connections[network->filled++] = conn;

  return network->connections + filled + 1;
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
      checksum((uint8_t *)&outPacketHeader, packetOffset);
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
  outSegmentHeader.checksum = checksum(payload, segmentHeaderSize);

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
  outSegmentHeader.checksum = checksum(payload, segmentSize);
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
  memcpy(outPacket.sourceHardwareAddress, network->tapdevice->mac,
         sizeof(outPacket.sourceHardwareAddress));
  outPacket.sourceProtocolAddress = packet.destProtocolAddress;
  memcpy(outPacket.destHardwareAddress, packet.sourceHardwareAddress, 6);
  outPacket.destProtocolAddress = packet.sourceProtocolAddress;

  TNET_ethSend(sock, frame, TNET_ETHERNET_TYPE_ARP, (uint8_t *)&outPacket,
               sizeof(outPacket));
}

void TNET_network_Network_serve(TNET_network_Network *network) {
  uint8_t ethernetBytes[sizeof(TNET_EthernetFrame)] = {0};
  int err;
  TNET_EthernetFrame *frame;
  TNET_TCPIPv4Connection *conn;

  DEBUG("Listening");

  while (1) {
    network->tapdevice->Read(network->tapdevice, &ethernetBytes,
                             sizeof(ethernetBytes));

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

    network->tcpipv4->Serve(network->tcpipv4, frame);
  }
}

int TNET_network_Network_Init(TNET_network_Network *network,
                              TNET_tapdevice_Tapdevice *tapdevice, int count) {
  network->connections =
      (TNET_TCPIPv4Connection *)malloc(sizeof(TNET_TCPIPv4Connection) * count);

  if (connections == NULL) {
    return -1;
  }

  network->count = count;
  network->filled = 0;
  network->tapdevice = tapdevice;
  network->Serve = TNET_network_Network_serve;
  network->Cleanup = TNET_network_Network_cleanup;

  srand(time(NULL));

  return 0;
}

void TNET_network_Network_cleanup(TNET_network_Network *network) {
  free(network->connections);
}
