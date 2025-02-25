#include "main_drivers.h"
#include "cc1101_config.h"
#include "helper_functions.h"

// Define SPI device paths
constexpr const char* SPI0_DEV = "/dev/spidev0.0";
constexpr const char* SPI1_DEV = "/dev/spidev1.0";

int main() {
    // Open SPI devices
    int spi0_fd = openSPI(SPI0_DEV);
    int spi1_fd = openSPI(SPI1_DEV);

    testConnections(spi0_fd, spi1_fd);

    print_MDMCFGs(spi0_fd);
    print_MDMCFGs(spi1_fd);

    printSyncPkt(spi0_fd);
    printSyncPkt(spi1_fd);

    // recordToFile(spi0_fd, spi1_fd, "longRecording.csv", 10'000);    // takes ~3.3 seconds = ~3000 samples per sec?

    // Close SPI devices
    close(spi0_fd);
    close(spi1_fd);

    return 0;
}
