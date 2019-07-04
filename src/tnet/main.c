#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "tnet/tap.h"
#include "tnet/tcp.h"
#include "tnet/types.h"

// SOURCE: https://stackoverflow.com/a/77336/1507139
void generateBacktrace(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main() {
  // Set up segfault backtracing
  struct sigaction action = {0};
  action.sa_handler = generateBacktrace;
  if (sigaction(SIGSEGV, &action, NULL) < 0) {
    printf("TNET: Unable to register segfault handler.\n");
    return 1;
  }

  int tap;
  char ifname[IFNAMSIZ];
  int err = TNET_tapInit(&tap, ifname);
  if (err != 0) {
    printf("TNET: Error initializing TAP device.\n");
    return err;
  }

  int nConnections = 2;
  err = TNET_tcpInit(nConnections, ifname);
  if (err != 0) {
    printf("TNET: Error initializing TCP/IP stack.\n");
    return err;
  }

  TNET_tcpServe(tap);

  TNET_tapCleanup(tap);

  return 0;
}
