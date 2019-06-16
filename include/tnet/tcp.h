#ifndef TNET_TCP_H
#define TNET_TCP_H

#include "tnet/types.h"

void TNET_tcpSynAck(int *connection, IPv4PacketHeader *packetHeader,
                    TCPSegmentHeader *segmentHeader);

void TNET_tcpSend(int *connection, IPv4PacketHeader *packetHeader,
                  TCPSegmentHeader *segmentHeader);

void TNET_tcpServe(int *connection);

void TNET_tcpInit();

void TNET_tcpCleanup();

#endif
