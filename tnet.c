#include <stdint.h>

int main() {
  tcpInitState();

  int fd;
  int err = tapInit(&fd);
  if (err != 0) {
    return err;
  }

  tcpServe(fd);

  close(fd);

  return;
}
