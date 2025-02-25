#include <string.h>             // memset()
#include <stdio.h>              // printf(), perror()
#include <unistd.h>             // write(), usleep()
#include <chrono>

#include <linux/spi/spidev.h>   // spi_ioc_transfer, SPI_IOC_WR_MODE, ...
#include <sys/ioctl.h>          // ioctl()
#include <fcntl.h>              // open(), O_RDWR, ...

#include "main_drivers.h"       // includes <stdint.h> for uint8_t, ...
#include "helper_functions.h"
#include "cc1101_config.h"      // includes <stdint.h>
#include "ansi_colors.h"


// device = path to spidevX.X
int openSPI(const char* device) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        return -1;
    }

    // Set SPI mode (0 = CPOL: SCLK idle LOW, CPHA: sample/read on rising edge)
    uint8_t mode = SPI_MODE_0;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    
    // Set bits per word (cc1101 SPI word = 1 R/W bit, 1 burst access bit, and 6 bit address)
    uint8_t bits = 8;
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    
    // Set SPI speed (cc110 has 10MHz max SCLK freq)
    uint32_t speed = 10'000'000;
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    return fd;
}

// cc1101MemoryOffset (pg. 70):
//      read single  = 0x80 (1000 0000) = only read reg param
//      read burst   = 0xC0 (1100 0000) = start reading at reg param
//      BUT SOME REGS REQUIRE BURST BIT ALWAYS (SEE STATUS/STROBE REGS)
uint8_t readRegister(int fd, uint8_t reg, uint8_t cc1101MemoryOffset, uint8_t numRegisters, uint8_t *returnBuff) {
    uint8_t txBuff[numRegisters + 1];   // starting address + placeholders to generate SCLK
    uint8_t rxBuff[numRegisters + 1];

    memset(txBuff, 0, numRegisters + 1);    // zero the buffer
    txBuff[0] = reg | cc1101MemoryOffset;   // set burst bit

    struct spi_ioc_transfer spi;        // spidev.h
    memset(&spi, 0, sizeof(spi));       // set struct fields to zero
    spi.tx_buf = (unsigned long)txBuff;     // set pointer to tx buffer
    spi.rx_buf = (unsigned long)rxBuff;     // set pointer to rx buffer
    spi.len = numRegisters + 1;
    
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &spi) < 0) {
        perror("SPI transfer failed");
        return 0;
    }

    if (numRegisters == 1) return rxBuff[1];  // first byte rx'd was sent from slave while it rx'd the address, so it needs a cycle to respond
    else {
        memcpy(returnBuff, &rxBuff[1], numRegisters); // ignore first byte
        return 1;
    }
}

// cc1101MemoryOffset (pg. 70):
//      write single = 0x00
//      write burst  = 0x40 (0100 0000)
//      SOME REGISTERS REQUIRE BURST BIT SET ALWAYS (SEE PAGE 70)
void writeRegister(int fd, uint8_t reg, uint8_t *data, uint8_t cc1101MemoryOffset, uint8_t numRegisters) {
    uint8_t txBuff[numRegisters + 1];   // address + data

    txBuff[0] = reg | cc1101MemoryOffset;   // set burst bit
    memcpy(&txBuff[1], data, numRegisters); // copy data to tx into buffer
    
    struct spi_ioc_transfer spi;
    memset(&spi, 0, sizeof(spi));

    spi.tx_buf = (unsigned long)txBuff;
    spi.len = numRegisters + 1;

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &spi) < 0) {
        perror("SPI write failed");
    }
}

// send byte strobe/command using write syscall
void sendStrobe(int fd, uint8_t strobe) {
    write(fd, &strobe, 1);
    usleep(500);  // allow the command to process/state to change
}

// try to read PARTNUM and VERSION registers and print them
void testConnections(int spi0_fd, int spi1_fd) {
    uint8_t partnum1 = readRegister(spi0_fd, PARTNUM, READ_BURST, 1, NULL);
    uint8_t version1 = readRegister(spi0_fd, VERSION, READ_BURST, 1, NULL);
    uint8_t partnum2 = readRegister(spi1_fd, PARTNUM, READ_BURST, 1, NULL);
    uint8_t version2 = readRegister(spi1_fd, VERSION, READ_BURST, 1, NULL);

    printf("CC1101 #1 (SPI0) - PARTNUM: 0x%02X, VERSION: 0x%02X\n", partnum1, version1);
    printf("CC1101 #2 (SPI1) - PARTNUM: 0x%02X, VERSION: 0x%02X\n", partnum2, version2);
}


// num_samples: number of RSSI values (32-bit/4-byte floats) to record (stack limited to ~8MB)
void recordToFile(int spi0_fd, int spi1_fd, const char *filename, int num_samples) {
    // set max rx fifo
    uint8_t maxFifo = 0x0F;
    writeRegister(spi0_fd, FIFOTHR, &maxFifo, WRITE_SINGLE_BYTE, 1);
    writeRegister(spi1_fd, FIFOTHR, &maxFifo, WRITE_SINGLE_BYTE, 1);

    // buffer to hold rssi values to write to file
    float rssiBuffer[num_samples];
    int   index = 0; 

    FILE *csvFile = fopen(filename, "w");
    if (!csvFile) {
        fprintf(stderr, "ERROR: Could not open %s for writing\n", filename);
        close(spi0_fd); close(spi1_fd);
        return;
    }

    // put both cc1101s in RX
    sendStrobe(spi0_fd, SRX);
    sendStrobe(spi1_fd, SRX); 
    
    bool recording = true;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (recording) {
        // check marcstate and rxbytes for rx fifo overflow
        uint8_t marcstate0 = readRegister(spi0_fd, MARCSTATE, READ_BURST, 1, NULL) & 0x1F;
        uint8_t marcstate1 = readRegister(spi1_fd, MARCSTATE, READ_BURST, 1, NULL) & 0x1F;
        uint8_t rxbytes0 = readRegister(spi0_fd, RXBYTES, READ_BURST, 1, NULL) & 0x7F;
        uint8_t rxbytes1 = readRegister(spi1_fd, RXBYTES, READ_BURST, 1, NULL) & 0x7F;

        if (marcstate0 != 0x0D || rxbytes0 > 60) {
            printf("SPI0: RXBYTES = %d, MARCSTATE = 0x%02X\n", rxbytes0, marcstate0);
            sendStrobe(spi0_fd, SFRX); // Flush RX FIFO
            sendStrobe(spi0_fd, SRX); // Re-enter RX mode
        }
        if (marcstate1 != 0x0D || rxbytes1 > 60) {
            printf("SPI1: RXBYTES = %d, MARCSTATE = 0x%02X\n", rxbytes1, marcstate1);
            sendStrobe(spi1_fd, SFRX);
            sendStrobe(spi1_fd, SRX);
        }

        usleep(500);  // .5ms

        uint8_t rssi_raw_0 = readRegister(spi0_fd, RSSI, READ_BURST, 1, NULL);
        uint8_t rssi_raw_1 = readRegister(spi1_fd, RSSI, READ_BURST, 1, NULL);
        float rssi_dbm_0 = convertRSSI(rssi_raw_0);
        float rssi_dbm_1 = convertRSSI(rssi_raw_1);

        printf("RSSI (SPI0): %.2f dBm | RSSI (SPI1): %.2f dBm\n", rssi_dbm_0, rssi_dbm_1);

        rssiBuffer[index++] = rssi_dbm_0;
        rssiBuffer[index++] = rssi_dbm_1;

        if (index >= num_samples) {
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;
            printf("Recorded for %.1f seconds\n", duration.count());

            for (int i = 0; i < num_samples; i++) {
                fprintf(csvFile, "%.2f\n", rssiBuffer[i]);
            }

            recording = false;
        }
    }
}

