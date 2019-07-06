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

  TNET_tapdevice_Tapdevice tapdevice;
  uint32_t address = inet_addr("192.168.2.3");
  uint32_t gateway = inet_addr("192.168.2.1");
  int error = TNET_tapdevice_Tapdevice_Init(&tapdevice, address, gateway);
  if (error != 0) {
    printf("TNET: Error initializing TAP device.\n");
    return err;
  }

  TNET_network_Network network;
  int nConnections = 2;
  error = TNET_network_Network_Init(&network, &tapdevice, nConnections);
  if (error != 0) {
    printf("TNET: Error initializing TCP/IP stack.\n");
    return err;
  }

  network.Serve(&network);

  network.Cleanup(&network);
  tapdevice.Cleanup(&tapdevice);

  return 0;
}
