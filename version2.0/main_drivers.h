#ifndef MAIN_DRIVERS_H
#define MAIN_DRIVERS_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

int openSPI(const char* device);
uint8_t readRegister(int fd, uint8_t reg, uint8_t cc1101MemoryOffset, uint8_t numRegisters, uint8_t *returnBuff);
void writeRegister(int fd, uint8_t reg, uint8_t *data, uint8_t cc1101MemoryOffset, uint8_t numRegisters);
void sendStrobe(int fd, uint8_t strobe);

void testConnections(int fd1, int fd2);
void recordToFile(int spi0_fd, int spi1_fd, const char *filename, int num_samples);

#endif