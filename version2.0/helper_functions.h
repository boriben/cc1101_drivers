#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <stdint.h>

// equations
float calculateDataRate(uint8_t drate_e, uint8_t drate_m);
uint32_t calculateChanSpc(uint8_t chanspc_e, uint8_t chanspc_m);
uint32_t calculateChanBW(uint8_t chanbw_e, uint8_t chanbw_m);
float convertRSSI(uint8_t hexRSSI);

// debugging
void getOrPrintBinary(uint8_t num, int bits, char *output);
void print_MDMCFGs(int fd);
void printSyncPkt(int fd);

#endif