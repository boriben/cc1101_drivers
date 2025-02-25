#ifndef CC1101_CONFIG_H
#define CC1101_CONFIG_H

#include <stdint.h>

// pins defined by BCM
// first CC1101
constexpr int SS_PIN = 24;
constexpr int GDO2 =   22;


// misc regs
constexpr uint32_t CRYSTAL_FREQUENCY        = 26'000'000;
constexpr uint8_t CFG_REGISTER              = 0x2F;
constexpr uint8_t FIFOBUFFER                = 0x42;
constexpr uint8_t RSSI_OFFSET_315MHZ        = 0x40;
constexpr uint8_t RSSI_OFFSET_433MHZ        = 0x41;
constexpr uint8_t RSSI_OFFSET_868MHZ        = 0x4A;
constexpr uint8_t RSSI_OFFSET_915MHZ        = 0x48;
constexpr uint8_t CC1100_COMPARE_REGISTER   = 0x00;
constexpr uint8_t BROADCAST_ADDRESS         = 0x00;
constexpr uint8_t CC1100_FREQ_315MHZ        = 0x01;
constexpr uint8_t CC1100_FREQ_434MHZ        = 0x02;
constexpr uint8_t CC1100_FREQ_868MHZ        = 0x03;
constexpr uint8_t CC1100_FREQ_915MHZ        = 0x04;
constexpr float CC1100_TEMP_ADC_MV        = 3.225;
constexpr float CC1100_TEMP_CELS_CO       = 2.47;

// read & write offsets
constexpr uint8_t WRITE_SINGLE_BYTE   = 0x00;
constexpr uint8_t WRITE_BURST         = 0x40;
constexpr uint8_t READ_SINGLE_BYTE    = 0x80;
constexpr uint8_t READ_BURST          = 0xC0;

// FIFO commands
constexpr uint8_t TXFIFO_BURST        = 0x7F;    // write burst only
constexpr uint8_t TXFIFO_SINGLE_BYTE  = 0x3F;    // write single only
constexpr uint8_t RXFIFO_BURST        = 0xFF;    // read burst only
constexpr uint8_t RXFIFO_SINGLE_BYTE  = 0xBF;    // read single only
constexpr uint8_t PATABLE_BURST       = 0x7E;    // power control read/write
constexpr uint8_t PATABLE_SINGLE_BYTE = 0xFE;    // power control read/write

// config regs
constexpr uint8_t IOCFG2   = 0x00;         // GDO2 output pin configuration
constexpr uint8_t IOCFG1   = 0x01;         // GDO1 output pin configuration
constexpr uint8_t IOCFG0   = 0x02;         // GDO0 output pin configuration
constexpr uint8_t FIFOTHR  = 0x03;         // RX FIFO and TX FIFO thresholds
constexpr uint8_t SYNC1    = 0x04;         // Sync word, high byte
constexpr uint8_t SYNC0    = 0x05;         // Sync word, low byte
constexpr uint8_t PKTLEN   = 0x06;         // Packet length
constexpr uint8_t PKTCTRL1 = 0x07;         // Packet automation control
constexpr uint8_t PKTCTRL0 = 0x08;         // Packet automation control
constexpr uint8_t ADDR     = 0x09;         // Device address
constexpr uint8_t CHANNR   = 0x0A;         // Channel number
constexpr uint8_t FSCTRL1  = 0x0B;         // Frequency synthesizer control
constexpr uint8_t FSCTRL0  = 0x0C;         // Frequency synthesizer control
constexpr uint8_t FREQ2    = 0x0D;         // Frequency control word, high byte
constexpr uint8_t FREQ1    = 0x0E;         // Frequency control word, middle byte
constexpr uint8_t FREQ0    = 0x0F;         // Frequency control word, low byte
constexpr uint8_t MDMCFG4  = 0x10;         // Modem configuration
constexpr uint8_t MDMCFG3  = 0x11;         // Modem configuration
constexpr uint8_t MDMCFG2  = 0x12;         // Modem configuration
constexpr uint8_t MDMCFG1  = 0x13;         // Modem configuration
constexpr uint8_t MDMCFG0  = 0x14;         // Modem configuration
constexpr uint8_t DEVIATN  = 0x15;         // Modem deviation setting
constexpr uint8_t MCSM2    = 0x16;         // Main Radio Cntrl State Machine config
constexpr uint8_t MCSM1    = 0x17;         // Main Radio Cntrl State Machine config
constexpr uint8_t MCSM0    = 0x18;         // Main Radio Cntrl State Machine config
constexpr uint8_t FOCCFG   = 0x19;         // Frequency Offset Compensation config
constexpr uint8_t BSCFG    = 0x1A;         // Bit Synchronization configuration
constexpr uint8_t AGCCTRL2 = 0x1B;         // AGC control
constexpr uint8_t AGCCTRL1 = 0x1C;         // AGC control
constexpr uint8_t AGCCTRL0 = 0x1D;         // AGC control
constexpr uint8_t WOREVT1  = 0x1E;         // High byte Event 0 timeout
constexpr uint8_t WOREVT0  = 0x1F;         // Low byte Event 0 timeout
constexpr uint8_t WORCTRL  = 0x20;         // Wake On Radio control
constexpr uint8_t FREND1   = 0x21;         // Front end RX configuration
constexpr uint8_t FREND0   = 0x22;         // Front end TX configuration
constexpr uint8_t FSCAL3   = 0x23;         // Frequency synthesizer calibration
constexpr uint8_t FSCAL2   = 0x24;         // Frequency synthesizer calibration
constexpr uint8_t FSCAL1   = 0x25;         // Frequency synthesizer calibration
constexpr uint8_t FSCAL0   = 0x26;         // Frequency synthesizer calibration
constexpr uint8_t RCCTRL1  = 0x27;         // RC oscillator configuration
constexpr uint8_t RCCTRL0  = 0x28;         // RC oscillator configuration
constexpr uint8_t FSTEST   = 0x29;         // Frequency synthesizer cal control
constexpr uint8_t PTEST    = 0x2A;         // Production test
constexpr uint8_t AGCTEST  = 0x2B;         // AGC test
constexpr uint8_t TEST2    = 0x2C;         // Various test settings
constexpr uint8_t TEST1    = 0x2D;         // Various test settings
constexpr uint8_t TEST0    = 0x2E;         // Various test settings

// command strobes
constexpr uint8_t SRES     = 0x30;         // Reset chip
constexpr uint8_t SFSTXON  = 0x31;         // Enable/calibrate freq synthesizer
constexpr uint8_t SXOFF    = 0x32;         // Turn off crystal oscillator.
constexpr uint8_t SCAL     = 0x33;         // Calibrate freq synthesizer & disable
constexpr uint8_t SRX      = 0x34;         // Enable RX.
constexpr uint8_t STX      = 0x35;         // Enable TX.
constexpr uint8_t SIDLE    = 0x36;         // Exit RX / TX
constexpr uint8_t SAFC     = 0x37;         // AFC adjustment of freq synthesizer
constexpr uint8_t SWOR     = 0x38;         // Start automatic RX polling sequence
constexpr uint8_t SPWD     = 0x39;         // Enter pwr down mode when CSn goes hi
constexpr uint8_t SFRX     = 0x3A;         // Flush the RX FIFO buffer.
constexpr uint8_t SFTX     = 0x3B;         // Flush the TX FIFO buffer.
constexpr uint8_t SWORRST  = 0x3C;         // Reset real time clock.
constexpr uint8_t SNOP     = 0x3D;         // No operation.
constexpr uint8_t TXRXFIFO = 0x3F;         // Tx and Rx FIFOs

// status regs (READ BURST OFFSET 0xC0 ALREADY INCLUDED/OR'd IN)
constexpr uint8_t PARTNUM        = 0x30;    // (0xF0)  Part number
constexpr uint8_t VERSION        = 0x31;    // (0xF1)  Current version number
constexpr uint8_t FREQEST        = 0x32;    // (0xF2)  Frequency offset estimate
constexpr uint8_t LQI            = 0x33;    // (0xF3)  Demodulator estimate for link quality
constexpr uint8_t RSSI           = 0x34;    // (0xF4)  Received signal strength indication
constexpr uint8_t MARCSTATE      = 0x35;    // (0xF5)  Control state machine state
constexpr uint8_t WORTIME1       = 0x36;    // (0xF6)  High byte of WOR timer
constexpr uint8_t WORTIME0       = 0x37;    // (0xF7)  Low byte of WOR timer
constexpr uint8_t PKTSTATUS      = 0x38;    // (0xF8)  Current GDOx status and packet status
constexpr uint8_t VCO_VC_DAC     = 0x39;    // (0xF9)  Current setting from PLL cal module
constexpr uint8_t TXBYTES        = 0x3A;    // (0xFA)  Underflow and # of bytes in TXFIFO
constexpr uint8_t RXBYTES        = 0x3B;    // (0xFB)  Overflow and # of bytes in RXFIFO
constexpr uint8_t RCCTRL1_STATUS = 0x3C;    // (0xFC)  Last RC Oscillator Calibration Result
constexpr uint8_t RCCTRL0_STATUS = 0x3D;    // (0xFD)  Last RC Oscillator Calibration Result

extern const uint8_t cc1100_GFSK_1_2_kb[CFG_REGISTER];
extern const uint8_t cc1100_GFSK_38_4_kb[CFG_REGISTER];
extern const uint8_t cc1100_GFSK_100_kb[CFG_REGISTER];
extern const uint8_t cc1100_MSK_250_kb[CFG_REGISTER];
extern const uint8_t cc1100_MSK_500_kb[CFG_REGISTER];
extern const uint8_t cc1100_OOK_4_8_kb[CFG_REGISTER];

extern const uint8_t patable_power_315[8];
extern const uint8_t patable_power_433[8];
extern const uint8_t patable_power_868[8];
extern const uint8_t patable_power_915[8];

#endif