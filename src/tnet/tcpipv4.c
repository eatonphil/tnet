TNET_tcpipv4_TCPIPv4_Serve(TNET_tcpivp4_TCPIPv4 *tcpipv4,
                           TNET_ethernet_Frame *frame) {
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
