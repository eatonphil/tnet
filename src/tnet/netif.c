#include "tnet/netif.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

int TNET_netif_setFlags(ifreq *ifr, short flags) {
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

// Queries for the mac address and assigns it on netif.
// Source:
// https://www.cnx-software.com/2011/04/05/c-code-to-get-mac-address-and-ip-address/
int TNET_netif_Netif_setMacAddress(TNET_netif_Netif *netif) {
  struct ifreq ifr = {0};
  strcpy(ifr.ifr_name, ifname);
  ifr.ifr_addr.sa_family = AF_INET;

  // Fill out MAC address
  int querySock = socket(AF_INET, SOCK_DGRAM, 0);
  if (ioctl(querySock, SIOCGIFHWADDR, &ifr) < 0) {
    DEBUG("Bad ioctl request for hardware address");
    return -1;
  }

  memcpy(netif->mac, ifr.ifr_hwaddr.sa_data, sizeof(netif->mac));
  return 0;
}

int TNET_netif_Netif_setIPv4Address(TNET_netif_Netif *netif, uint32_t address) {
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

int TNET_netif_Netif_setIPv4Gateway(TNET_netif_Netif *netif, uint32_t gateway) {
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

int TNET_netif_Netif_initDevice(TNET_netif_Netif *netif) {
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
  TNET_netif_Netif_setFlags(&ifr, IFF_UP | IFF_RUNNING);
  memcpy(netif->ifname, ifr.ifr_name, IFNAMSIZ);
  netif->fd = tmpFd;
}

int TNET_netif_Netif_Init(TNET_netif_Netif *netif, uint32_t ipv4Address,
                          uint32_t ipv4Gateway) {
  int error = TNET_netif_Netif_initDevice(netif);
  if (error != 0) {
    return error;
  }

  error = TNET_netif_Netif_setMacAddress(netif);
  if (error != 0) {
    return error;
  }

  error = TNET_netif_Netif_setIPv4Address(netif, ipv4Address);
  if (error != 0) {
    return error;
  }

  error = TNET_netif_Netif_setIPv4Gateway(netif, ipv4Gateway);

  netif->Read = TNET_netif_Netif_read;
  netif->Cleanup = TNET_netif_Netif_cleanup;

  return 0;
}

int TNET_netif_Netif_read(TNET_netif_Netif *netif, uint8_t *buffer,
                          uint32_t bufferSize) {
  return read(netif->fd, &buffer, bufferSize);
}

void TNET_netif_Netif_cleanup(TNET_netif_Netif *netif) { close(netif->fd); }
