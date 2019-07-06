#ifndef TNET_TAP_H
#define TNET_TAP_H

#include <stdint.h>

#include <net/if.h>

typedef struct {
  // Private
  char ifname[IFNAMSIZ];
  int fd;
  uint32_t ipv4Address;

  // Public
  uint8_t Mac[6];
  int (*Read)(TNET_tapdevice_Tapdevice *tapdevice, uint8_t *buffer,
              int bufferSize);
  void (*Cleanup)(TNET_tapdevice_Tapdevice *tapdevice);
} TNET_tapdevice_Tapdevice;

int TNET_tapdevice_Tapdevice_Init(TNET_tapdevice_Tapdevice *);

#endif
