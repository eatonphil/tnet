#include "tnet/tap.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int TNET_tapSetFlags(ifreq *ifr, short flags) {
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

int TNET_tapInit(int *fd, char ifname[IFNAMSIZ]) {
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
  TNET_tapSetFlags(&ifr, IFF_UP | IFF_RUNNING);

  // Return ifname
  memcpy(ifname, ifr.ifr_name, IFNAMSIZ);

  *fd = tmpFd;
  return 0;
}

void TNET_tapCleanup(int fd) { close(fd); }
