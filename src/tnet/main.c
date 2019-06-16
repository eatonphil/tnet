#include "tnet/tap.h"
#include "tnet/tcp.h"
#include "tnet/types.h"

int main() {
  TNET_tcpInit();

  int tap;
  int err = TNET_tapInit(&tap);
  if (err != 0) {
    return err;
  }

  TNET_tcpServe(tap);

  TNET_tapCleanup(tap);

  return;
}
