#include "tnet/tcpipv4.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "tnet/ethernet.h"
#include "tnet/util.h"

TNET_tcpipv4_Connection *
TNET_tcpipv4_TCPIPv4_getConnection(TNET_tcpipv4_TCPIPv4 *tcpipv4,
                                   TNET_EthernetFrame *frame) {
  TNET_IPv4PacketHeader packetHeader = {0};
  packetHeader = TNET_IPv4_PACKET_FROM_ETHERNET_FRAME(frame);
  TNET_TCPSegmentHeader segmentHeader = {0};
  segmentHeader = TNET_TCP_SEGMENT_FROM_ETHERNET_FRAME(frame);

  TNET_tcpipv4_Connection conn = {0};
  int i = 0;
  for (; i < tcpipv4->filled; i++) {
    conn = tcpipv4->connections[i];
    if (conn.sourceIPAddress == packetHeader.sourceIPAddress &&
        conn.sourcePort == segmentHeader.sourcePort &&
        conn.destIPAddress == packetHeader.destIPAddress &&
        conn.destPort == segmentHeader.destPort) {
      return tcpipv4->connections + i;
    }
  }

  if (i != tcpipv4->count - 1) {
    return NULL;
  }

  return NULL;
}

TNET_tcpipv4_Connection *
TNET_tcpipv4_NewConnection(TNET_EthernetFrame *frame, int sock,
                           uint16_t initialSequenceNumber) {
  int filled = tcpipv4->filled;
  int count = tcpipv4->count;
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
  tcpipv4->connections[tcpipv4->filled++] = conn;

  return tcpipv4->connections + filled + 1;
}

void TNET_tcpipv4_TCPIPv4_serve(TNET_tcpipv4_TCPIPv4 *tcpipv4,
                                TNET_netif_Netif *netif,
                                TNET_ethernet_Frame *frame) {
  TNET_tcpipv4_Connection conn = {0};
  conn = TNET_tcpipv4_getConnection(frame);
  if (conn == NULL) {
    conn = TNET_tcpipv4_NewConnection(frame, sock, rand());
    if (conn == NULL) {
      break;
    }
  }

  switch (conn->state) {
  case TNET_TCP_STATE_CLOSED:
    break;
  case TNET_TCP_STATE_LISTEN:
    TNET_DEBUG("Sending syn-ack");
    TNET_tcpSynAck(conn, frame);
    TNET_DEBUG("Sent syn-ack");
    conn->state = TNET_TCP_STATE_SYN_RECEIVED;
    break;
  case TNET_TCP_STATE_SYN_RECEIVED:
    TNET_DEBUG("Sending data");
    TNET_tcp_Send(conn, frame);
    TNET_DEBUG("Sent data");
    conn->state = TNET_TCP_STATE_ESTABLISHED;
    break;
  case TNET_TCP_STATE_ESTABLISHED:
    // Ignore any incoming frames for now.
    break;
  }
}

void TNET_tcpipv4_TCPIPv4_cleanup(TNET_tcpipv4_TCPIPv4 *tcpipv4) {
  free(tcpipv4->connections);
}

int TNET_tcpipv4_TCPIPv4_Init(TNET_tcpipv4_TCPIPv4 *tcpipv4, int count) {
  tcpipv4->connections = (TNET_tcpipv4_Connection *)malloc(
      sizeof(TNET_tcpipv4_Connection) * count);
  if (tcpipv4->connections == NULL) {
    return -1;
  }

  tcpipv4->count = count;
  tcpipv4->filled = 0;
  tcpipv4->Cleanup = TNET_tcpipv4_TCPIPv4_cleanup;
  tcpipv4->Serve = TNET_tcpipv4_TCPIPv4_serve;

  return 0;
}
