#include "tnet/tapdevice.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int TNET_tapdevice_setFlags(ifreq *ifr, short flags) {
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

// Queries for the mac address and assigns it on tapdevice.
// Source:
// https://www.cnx-software.com/2011/04/05/c-code-to-get-mac-address-and-ip-address/
int TNET_tapdevice_Tapdevice_setMacAddress(
    TNET_tapdevice_Tapdevice *tapdevice) {
  struct ifreq ifr = {0};
  strcpy(ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;

  // Fill out MAC address
  int querySock = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl(querySock, SIOCGIFHWADDR, &ifr) < 0) {
    DEBUG("Bad ioctl request for hardware address");
    return -1;
  }

  memcpy(tapdevice->mac, ifr.ifr_hwaddr.sa_data, sizeof(tapdevice->mac));
  return 0;
}

int TNET_tapdevice_Tapdevice_setIPv4Address(TNET_tapdevice_Tapdevice *tapdevice,
                                            uint32_t address) {
  // Fill out IP address
  struct ifreq ifr2 = {0};
  strcpy(ifr2.ifr_name, ifname);
  struct sockaddr_in sai = {0};
  sai.sin_family = AF_INET;
  sai.sin_port = 0;
  sai.sin_addr.s_addr = address; // inet_addr("192.168.2.3");
  ifr2.ifr_addr = *((sockaddr *)&sai);

  if (ioctl(querySock, SIOCSIFADDR, &ifr2) < 0) {
    DEBUG("Bad ioctl request to set protocol address");
    return 0;
  }
}

int TNET_tapdevice_Tapdevice_setIPv4Gateway(TNET_tapdevice_Tapdevice *tapdevice,
                                            uint32_t gateway) {
  // Set gateway
  struct rtentry route = {0};
  route.rt_flags = RTF_UP | RTF_GATEWAY;
  route.rt_dev = ifname;
  struct sockaddr_in *addr = (struct sockaddr_in *)&route.rt_gateway;
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = gateway;
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

  return 0;
}

int TNET_tapdevice_Tapdevice_initDevice(TNET_tapdevice_Tapdevice *tapdevice) {
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
  TNET_tapdevice_Tapdevice_setFlags(&ifr, IFF_UP | IFF_RUNNING);
  memcpy(tapdevice->ifname, ifr.ifr_name, IFNAMSIZ);
  tapdevice->fd = tmpFd;
}

int TNET_tapdevice_Tapdevice_Init(TNET_tapdevice_Tapdevice *tapdevice,
                                  uint32_t ipv4Address, uint32_t ipv4Gateway) {
  int error = TNET_tapdevice_Tapdevice_makeDevice(tapdevice);
  if (error != 0) {
    return error;
  }

  error = TNET_tapdevice_Tapdevice_setMacAddress(tapdevice);
  if (error != 0) {
    return error;
  }

  error = TNET_tapdevice_Tapdevice_setIPv4Address(tapdevice, ipv4Address);
  if (error != 0) {
    return error;
  }

  error = TNET_tapdevice_Tapdevice_setIPv4Gateway(tapdevice, ipv4Gateway);

  tapdevice->Read = TNET_tapdevice_Tapdevice_read;
  tapdevice->Cleanup = TNET_tapdevice_Tapdevice_cleanup;

  return 0;
}

int TNET_tapdevice_Tapdevice_read(TNET_tapdevice_Tapdevice *tapdevice,
                                  uint8_t *buffer, uint32_t bufferSize) {
  return read(tapdevice->fd, &buffer, bufferSize);
}

void TNET_tapdevice_Tapdevice_cleanup() { close(fd); }
