#include <stdio.h>

#include "tnet/tap.h"
#include "tnet/tcp.h"
#include "tnet/types.h"

int main() {
  int tap;
  char ifname[IFNAMSIZ];
  int err = TNET_tapInit(&tap, ifname);
  if (err != 0) {
    printf("TNET: Error initializing TAP device.\n");
    return err;
  }

  int nConnections = 2;
  err = TNET_tcpInit(nConnections);
  if (err != 0) {
    printf("TNET: Error initializing TCP/IP stack.\n");
    return err;
  }

  TNET_tcpServe(tap, ifname);

  TNET_tapCleanup(tap);

  return 0;
}
