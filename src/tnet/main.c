#include "tnet/tap.h"
#include "tnet/tcp.h"
#include "tnet/types.h"

int main() {
  TNET_tcpInit();

  int fd;
  int err = tapInit(&fd);
  if (err != 0) {
    return err;
  }

  tcpServe(fd);

  tapCleanup(fd);

  return;
}
