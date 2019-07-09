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

#include "tnet/arp.h"
#include "tnet/tcp.h"

#define TNET_DEBUG(str) printf("TNET: %s\n", str)

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

void TNET_network_Network_arpRespond(TNET_network_Network *network,
                                     TNET_network_ARPPacket *packet,
                                     TNET_network_ARPPacket *outPacket) {
  if (ntohs(packet.protocolType) != TNET_ETHERNET_TYPE_IPv4) {
    TNET_DEBUG("Dropping non-ipv4 arp request");
    return;
  }

  // Fill out rest
  outPacket->hardwareType = packet.hardwareType;
  outPacket->protocolType = packet.protocolType;
  outPacket->hardwareAddressLength = packet.hardwareAddressLength;
  outPacket->protocolAddressLength = packet.protocolAddressLength;
  outPacket->operation = htons(2); // Response
  memcpy(outPacket->sourceHardwareAddress, network->tapdevice->mac,
         sizeof(outPacket->sourceHardwareAddress));
  outPacket->sourceProtocolAddress = packet.destProtocolAddress;
  memcpy(outPacket->destHardwareAddress, packet.sourceHardwareAddress, 6);
  outPacket->destProtocolAddress = packet.sourceProtocolAddress;
}

void TNET_network_Network_serveArp(TNET_network_Network *network,
                                   TNET_network_EthernetFrame *frame) {
  TNET_network_ARPPacket arpPacket = {0};
  TNET_network_ARPPacket arpResponse = {0};
  arpPacket = TNET_ARP_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_DEBUG_ARP_PACKET(packet);
  TNET_network_Network_arpServe(network, &arpPacket, &arpResponse);
  TNET_network_Network_ethernetServe(network, &outFrame, &outFrameLength,
                                     (uint8_t *)&arpResponse,
                                     sizeof(outPacket));
  network->tapdevice->write(network->tapdevice, (uint8_t *)&outFrame,
                            frameLength);
}

void TNET_network_Network_serveIPv4(TNET_network_Network *network,
                                    TNET_ethernet_EthernetFrame *frame) {
  auto ipPacket = TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
  printf("TNET: Received packet version %d, protocol %d\n",
         TNET_IPv4_PACKET_VERSION(ipPacket), ipPacket.protocol);

  if (TNET_IPv4_PACKET_VERSION(ipPacket) != 4 ||
      ipPacket.protocol != TNET_IP_TYPE_TCP) {
    TNET_DEBUG("Dropping packet");
    continue;
  }

  TNET_DEBUG_IPv4_PACKET(frame);
  TNET_DEBUG_TCP_SEGMENT(frame);

  network->tcpipv4->Serve(network->tcpipv4, &frame);
}

void TNET_network_Network_serve(TNET_network_Network *network) {
  uint8_t ethernetBytes[sizeof(TNET_network_EthernetFrame)] = {0};
  int err;
  TNET_tcpipv4_Connection *conn;
  TNET_network_EthernetFrame frame = {0};
  TNET_network_EthernetFrame frameResponse = {0};
  int frameResponseLength = 0;

  TNET_DEBUG("Listening");

  while (1) {
    memset(frame, 0, sizeof(frame));
    memset(frameResponse, 0, sizeof(frameResponse));
    frameResponseLength = 0;

    network->tapdevice->Read(network->tapdevice, &ethernetBytes,
                             sizeof(ethernetBytes));

    frame = *(TNET_EthernetFrame *)&ethernetBytes;

    printf("TNET: Received frame of type %#06x\n", ntohs(frame.type));

    switch (ntohs(frame.type)) {
    case TNET_ETHERNET_TYPE_ARP:
      TNET_network_Network_handleArp(network, &frame);
      break;
    case TNET_ETHERNET_TYPE_IPv4:
      TNET_network_Network_handleIPv4(network, &frame);
      break;
    default:
      TNET_DEBUG("Dropping frame");
      break;
    }
  }
}

int TNET_network_Network_Init(TNET_network_Network *network,
                              TNET_tapdevice_Tapdevice *tapdevice, int count) {
  network->tcpipv4 = {0};
  if (TNET_tcpipv4_TCPIPv4_Init(&network->tcpipv4, count) == NULL) {
    return -1;
  }

  network->tapdevice = tapdevice;
  network->Serve = TNET_network_Network_serve;
  network->Cleanup = TNET_network_Network_cleanup;

  srand(time(NULL));

  return 0;
}

void TNET_network_Network_cleanup(TNET_network_Network *network) {
  network->tcpipv4->Cleanup();
}
