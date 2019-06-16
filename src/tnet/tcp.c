#include "tnet/tcp.h"

void TNET_tcpSynAck(int *connection, IPv4PacketHeader *packetHeader,
                    TCPSegmentHeader *segmentHeader) {}

void TNET_tcpSend(int *connection, IPv4PacketHeader *packetHeader,
                  TCPSegmentHeader *segmentHeader) {}

void TNET_tcpServe(int *connection) {
  while (1) {
    EthernetFrame frame = read(connection);
    IPv4PacketHeader packetHeader = (IPv4PacketHeader)frame.payload;
    TCPSegmentHeader segmentHeader =
        (TCPSegmentHeader)frame.payload[packetHeader.headerLength];

    switch (TCP_STATE(packetHeader, segmentHeader)) {
    case TCP_STATE_LISTEN:
      tcpSynAck(connection, &packetHeader, &segmentHeader);
      SET_TCP_STATE(packetHeader, segmentHeader, TCP_STATE_ESTABLISHED);
      return;
    case TCP_STATE_ESTABLISHED:
      tcpSend(connection, &packetHeader, &segmentHeader);
      SET_TCP_STATE(packetHeader, segmentHeader, TCP_STATE_ESTABLISHED);
      return;
    }
  }
}

void TNET_tcpInit() {}

void TNET_tcpCleanup() {}
