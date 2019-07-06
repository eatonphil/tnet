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
