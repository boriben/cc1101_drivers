#include "main_drivers.h"
#include "helper_functions.h"
#include "cc1101_config.h"
#include "ansi_colors.h"

#include <stdio.h>

// datasheet page 35
float calculateDataRate(uint8_t drate_e, uint8_t drate_m) {
    return ((256 + drate_m) * (1 << drate_e) * (float)CRYSTAL_FREQUENCY) / (1 << 28);
}

// see datasheet formula (pg. 78)
uint32_t calculateChanSpc(uint8_t chanspc_e, uint8_t chanspc_m) {
    return (26000000 / (1 << 18) * (256 + chanspc_m) * (1 << chanspc_e));
}

uint32_t calculateChanBW(uint8_t chanbw_e, uint8_t chanbw_m) {
    return CRYSTAL_FREQUENCY / (8 * (4 + chanbw_m) * (1 << chanbw_e));
}

// Convert raw RSSI to dBm
float convertRSSI(uint8_t hexRSSI) {
    int8_t signedRSSI = (int8_t)hexRSSI;  // Interpret as signed value
    return ((signedRSSI / 2.0) - 74.0);   // Convert to dBm
}


// return binary string by passing output string, or pass NULL to just print binary string
void getOrPrintBinary(uint8_t num, int bits, char *output) {
    for (int i = bits - 1; i >= 0; i--) {
        char bit = (num & (1 << i)) ? '1' : '0';

        if (output) {
            output[bits - 1 - i] = bit; 
        } else {
            printf("%c", bit);
        }
    }
    if (output) {
        output[bits] = '\0'; 
    }
    printf("\n");
}


void print_MDMCFGs(int fd) {
    uint8_t mdmcfg_regs[5];
    if (!readRegister(fd, MDMCFG4, READ_BURST, 5, mdmcfg_regs)) return;

    uint8_t mdmcfg4 = mdmcfg_regs[0];
    uint8_t drate_e = mdmcfg4 & 0x0F;     
    uint8_t chanbw_m = (mdmcfg4 >> 4) & 0x03; 
    uint8_t chanbw_e = (mdmcfg4 >> 6) & 0x03;

    uint8_t mdmcfg3 = mdmcfg_regs[1];

    uint8_t mdmcfg2 = mdmcfg_regs[2];
    uint8_t sync_mode = mdmcfg2 & 0x07;
    uint8_t manchester_en = (mdmcfg2 >> 3) & 0x01;
    uint8_t mod_format = (mdmcfg2 >> 4) & 0x07;
    uint8_t dem_dcfilt_off = (mdmcfg2 >> 7) & 0x01;

    uint8_t mdmcfg1 = mdmcfg_regs[3];
    uint8_t chanspc_e = mdmcfg1 & 0x03;
    uint8_t num_preamble = (mdmcfg1 >> 4) & 0x07;
    uint8_t fec_en = (mdmcfg1 >> 7) & 0x01;

    uint8_t mdmcfg0 = mdmcfg_regs[4];

    printf("====== MDMCFGs for fd: %d ======\n", fd);

    printf("%sMDMCFG4%s: 0x%02X = %u = ", BLUE, RESET, mdmcfg4, mdmcfg4);
    getOrPrintBinary(mdmcfg4, 8, NULL);
    printf("    %sDRATE_E%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, drate_e, drate_e);
    getOrPrintBinary(drate_e, 4, NULL);
    printf("    %sCHANBW_M%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, chanbw_m, chanbw_m);
    getOrPrintBinary(chanbw_m, 2, NULL);
    printf("    %sCHANBW_E%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, chanbw_e, chanbw_e);
    getOrPrintBinary(chanbw_e, 2, NULL);

    printf("%sMDMCFG3%s: 0x%02X = %u = ", BLUE, RESET, mdmcfg3, mdmcfg3);
    getOrPrintBinary(mdmcfg3, 8, NULL);
    printf("    %sDRATE_M%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, mdmcfg3, mdmcfg3);
    getOrPrintBinary(mdmcfg3, 8, NULL);

    printf("%sMDMCFG2%s: 0x%02X = %u = ", BLUE, RESET, mdmcfg2, mdmcfg2);
    getOrPrintBinary(mdmcfg2, 8, NULL);
    printf("    SYNC_MODE: 0x%02X = %u = ", sync_mode, sync_mode);
    getOrPrintBinary(sync_mode, 3, NULL);
    printf("    MANCHESTER_EN: 0x%02X = %u = ", manchester_en, manchester_en);
    getOrPrintBinary(manchester_en, 1, NULL);
    printf("    %sMOD_FORMAT%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, mod_format, mod_format);
    getOrPrintBinary(mod_format, 3, NULL);
    printf("    DEM_DCFILT_OFF: 0x%02X = %u = ", dem_dcfilt_off, dem_dcfilt_off);
    getOrPrintBinary(dem_dcfilt_off, 1, NULL);

    printf("%sMDMCFG1%s: 0x%02X = %u = ", BLUE, RESET, mdmcfg1, mdmcfg1);
    getOrPrintBinary(mdmcfg1, 8, NULL);
    printf("    %sCHANSPC_E%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, chanspc_e, chanspc_e);
    getOrPrintBinary(chanspc_e, 2, NULL);
    printf("    NUM_PREAMBLE: 0x%02X = %u = ", num_preamble, num_preamble);
    getOrPrintBinary(num_preamble, 3, NULL);
    printf("    FEC_EN: 0x%02X = %u = ", fec_en, fec_en);
    getOrPrintBinary(fec_en, 1, NULL);

    printf("%sMDMCFG0%s: 0x%02X = %u = ", BLUE, RESET, mdmcfg0, mdmcfg0);
    getOrPrintBinary(mdmcfg0, 8, NULL);
    printf("    %sCHANSCP_M%s: 0x%02X = %u = ", BG_BRIGHT_YELLOW, RESET, mdmcfg0, mdmcfg0);
    getOrPrintBinary(mdmcfg0, 8, NULL);
}

// SYNC1, SYNC0, PKTLEN, PKTCTRL1, and PKTCTRL0 regs
void printSyncPkt(int fd) {
    uint8_t regs[5];
    if (!readRegister(fd, SYNC1, READ_BURST, 5, regs)) return;

    uint8_t sync1 = regs[0];        // 8 MSB of 16 bit sync word (reset = 0xD3)

    uint8_t sync0 = regs[1];        // 8 LSB of 16 bit sync word (reset = 0x91)

    uint8_t pktlen = regs[2];       // if fixed pklen, this is pklen, if variable pklen, this is max pklen

    uint8_t pktctrl1 = regs[3];
    uint8_t append_status = (pktctrl1 >> 2) & 0x01;        // APPEND_STATUS at PKTCTRL1[2] controls RSSI/LQI appended to packets (reset = 1)
    uint8_t adr_check = pktctrl1 & 0x03;                 // ADR_CHECK at PKTCTRL1[1:0] (reset = 00 = no address check)

    uint8_t pktctrl0 = regs[4];
    uint8_t white_data = (pktctrl0 >> 6) & 0x01;           // WHITE_DATA at bit 6 (reset = 1)
    uint8_t pkt_format = (pktctrl0 >> 4) & 0x03;            // PKT_FORMAT at bits 5:4 (reset = 00 = normal mode)
    uint8_t crc_en = (pktctrl0 >> 2) & 0x01;        // reset = 1
    uint8_t length_config = pktctrl0 & 0x03;        // configure packet length, reset = 01 = variable packet length mode, 00 = fixed, 10 = inf

    printf("========== SYNC & PKT regs for fd: %d ==========", fd);

    printf("%sSYNC1%s: 0x%02X = %u = ", BLUE, RESET, sync1, sync1);
    getOrPrintBinary(sync1, 8, NULL);
    printf("%sSYNC0%s: 0x%02X = %u = ", BLUE, RESET, sync0, sync0);
    getOrPrintBinary(sync0, 8, NULL);
    printf("%sPKTLEN%s: 0x%02X = %u = ", BLUE, RESET, pktlen, pktlen);
    getOrPrintBinary(pktlen, 8, NULL);
    printf("%sPKTCTRL1%s: 0x%02X = %u = ", BLUE, RESET, pktctrl1, pktctrl1);
    getOrPrintBinary(pktctrl1, 8, NULL);
    printf("    %sAPPEND_STATUS%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, append_status, append_status);
    getOrPrintBinary(append_status, 1, NULL);
    printf("    %sADR_CHECK%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, adr_check, adr_check);
    getOrPrintBinary(adr_check, 2, NULL);
    printf("%sPKTCTRL0%s: 0x%02X = %u = ", BLUE, RESET, pktctrl0, pktctrl0);
    getOrPrintBinary(pktctrl0, 8, NULL);
    printf("    %sWHITE_DATA%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, white_data, white_data);
    getOrPrintBinary(white_data, 1, NULL);
    printf("    %sPKT_FORMAT%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, pkt_format, pkt_format);
    getOrPrintBinary(pkt_format, 2, NULL);
    printf("    %sCRC_EN%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, crc_en, crc_en);
    getOrPrintBinary(crc_en, 1, NULL);
    printf("    %sLENGTH_CONFIG%s: 0x%02X = %u = ", BG_BRIGHT_GREEN, RESET, length_config, length_config);
    getOrPrintBinary(length_config, 2, NULL);


}