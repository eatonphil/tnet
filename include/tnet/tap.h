#ifndef TNET_TAP_H
#define TNET_TAP_H

#include "net/if.h"

int TNET_tapSetFlags(ifreq *ifr, short flags);

int TNET_tapInit(int *fd);

void TNET_tapCleanup(int *fd);

#endif
