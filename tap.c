int tapSetFlags(ifreq *ifr, short flags) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  // Get current flags
  int err = ioctl(sock, SIOCGIFFLAGS, ifr);
  if (err == -1) {
    return errno;
  }

  ifr->ifr_flags |= flags;

  // Commit new flags
  err = ioctl(sock, SIOCSIFFLAGS, ifr);
  close(sock);
  if (err == -1) {
    return errno;
  }

  return 0;
}

int tapInit(int *fd) {
  int tmpFd = open("/dev/net/tun", O_RDWR);
  if (tmpFd == -1) {
    return errno;
  }

  ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  // Ethernet layer with no additional packet information
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

  int err = ioctl(tmpFd, TUNSETIFF, &ifr);
  if (err == -1) {
    return errno;
  }

  // Set up and running
  tapSetFlags(&ifr, IFF_UP | IFF_RUNNING);

  *fd = tmpFd;
  return 0;
}
