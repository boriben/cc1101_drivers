#include <unistd.h>  // for usleep()
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <algorithm> // std::max, std::min
#include <cmath>
#include <wiringPi.h>       // pinMode(), digitalWrite(), digitalRead(), wiringPiSetup(), ...
#include <wiringPiSPI.h>    // wiringPiSPISetup(), wiringPiSPIDataRW(), ... 
#include "cc1101_config.h"
#include "ansi_colors.h"

#define RESET   "\x1b[0m"  // Resets color to default
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 400

#define SPECTRUM_HEIGHT 200

#define WATERFALL_HEIGHT 200
#define WATERFALL_ROWS 100

#define NUM_CHANNELS 50     // num x values on spectrum (resolution)

std::vector<int16_t> rssi_values(NUM_CHANNELS, -100);
std::vector<std::vector<int16_t>> waterfall(WATERFALL_ROWS, std::vector<int16_t>(NUM_CHANNELS, -100));

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

void spi_write_strobe(uint8_t spi_instr) {
    uint8_t tbuf[1] = {0};
    tbuf[0] = spi_instr;
    //printf("SPI_data: 0x%02X\n", tbuf[0]);
    wiringPiSPIDataRW(0, tbuf, 1) ;
}

void spi_write_register(uint8_t spi_instr, uint8_t value) {
    uint8_t tbuf[2] = {0};
    tbuf[0] = spi_instr | WRITE_SINGLE_BYTE;
    tbuf[1] = value;
    wiringPiSPIDataRW(0, tbuf, 2) ;
    return;
}

void spi_write_burst(uint8_t spi_instr, const uint8_t *pArr, uint8_t len) {
     uint8_t tbuf[len + 1];
     tbuf[0] = spi_instr | WRITE_BURST;
     for (uint8_t i=0; i<len ;i++ )
     {
          tbuf[i+1] = pArr[i];
          //printf("SPI_arr_write: 0x%02X\n", tbuf[i+1]);
     }
     wiringPiSPIDataRW(0, tbuf, len + 1);
}

uint8_t spi_read_register(uint8_t regi) {
    uint8_t bytes[2] = {0};                      // two bytes: one for the register to read and one to store the result

    bytes[0] = regi | READ_SINGLE_BYTE;     // set R/W bit in command byte

    wiringPiSPIDataRW(0, bytes, 2);

    return bytes[1];
}

void spi_read_burst(uint8_t spi_instr, uint8_t *pArr, uint8_t len) {
    uint8_t rbuf[len + 1];
    rbuf[0] = spi_instr | READ_BURST;   // add read burst mask bit to front of instruction
    wiringPiSPIDataRW(0, rbuf, len + 1);    // read data into buffer
    for (uint8_t i = 0; i < len; i++) { pArr[i] = rbuf[i+1]; } // copy buffer to our array
}

void setupSPI() {
    wiringPiSetup();
    pinMode(GDO2, INPUT);
    pinMode(SS_PIN, OUTPUT);
    wiringPiSPISetup(0, 10000000); // 10MHz SPI speed

    uint8_t partnum, version;
    partnum = spi_read_register(PARTNUM);   // Datasheet says 0x00
    version = spi_read_register(VERSION);   // Datasheet says 0x14
    printf("Partnumber: 0x%02X, Version: 0x%02X\r\n", partnum, version);
}

// Pass references to window, renderer, and font (null)pointers
void setupSDL(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    *window = SDL_CreateWindow("spectrum analyzer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 12);
}

void sidle(void) {
    spi_write_strobe(SIDLE);
    delayMicroseconds(100);
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

// See page 76-78 of datasheet
void print_MDMCFGs() {
    uint8_t mdmcfg_regs[5];
    spi_read_burst(MDMCFG4, mdmcfg_regs, 5);

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

    printf("====== Modem Config Registers ======\n");

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

/*
Datasheet page 77/98:
MOD_FORMAT in 3 bits from MDMCFG2[6:4]
0 = 2FSK
1 = GFSK
3 = ASK/OOK
4 = 4FSK
7 = MSK
*/
void set_modulation_type(uint8_t mod_type) {
    uint8_t mdmcfg2 = spi_read_register(MDMCFG2);
    mdmcfg2 = mdmcfg2 | (mod_type << 4);
    spi_write_register(MDMCFG2, mdmcfg2);
}

const char* get_modulation_type() {
    // MOD_FORMAT bits [6:4] from MDMCFG2
    switch ((spi_read_register(MDMCFG2) >> 4) & 0x07) {
        case 0: return "2FSK";
        case 1: return "GFSK";
        case 3: return "ASK/OOK";
        case 4: return "4FSK";
        case 7: return "MSK";
        default: return "wtf idk";
    }
}

void set_freq(uint8_t ism_freq) {
    uint8_t freq2, freq1, freq0;
    switch (ism_freq) {
        case 0x01: // 315 Mhz
            freq2=0x0C;
            freq1=0x1D;
            freq0=0x89;
            // spi_write_burst(PATABLE_BURST, patable_power_315, 8); // only needed for TX
            break;
        case 0x02: // 433 MHz
                freq2=0x10;
                freq1=0xB0;
                freq0=0x71;
                // spi_write_burst(PATABLE_BURST, patable_power_433, 8);
                break;
        case 0x03: // 868 MHz
                freq2=0x21;
                freq1=0x65;
                freq0=0x6A;
                // spi_write_burst(PATABLE_BURST, patable_power_868, 8);
                break;
        case 0x04: // 915MHz
                freq2=0x23;
                freq1=0x31;
                freq0=0x3B;
                // spi_write_burst(PATABLE_BURST, patable_power_915, 8);
                break;
    }
    spi_write_register(FREQ2,freq2);
    spi_write_register(FREQ1,freq1);
    spi_write_register(FREQ0,freq0);
}

void set_specific_freq(double hz) {
    uint32_t freq_word = (uint32_t)((hz * 65536) / CRYSTAL_FREQUENCY);

    // 24 bit freq word split into 3 registers
    uint8_t freq2 = (freq_word >> 16) & 0xFF; // 8 biggest bits
    uint8_t freq1 = (freq_word >> 8) & 0xFF;
    uint8_t freq0 = freq_word & 0xFF; // 8 smallest/LSBs

    spi_write_register(FREQ2,freq2);
    spi_write_register(FREQ1,freq1);
    spi_write_register(FREQ0,freq0);
}

uint32_t get_frequency() {
    uint8_t freq0 = spi_read_register(FREQ0);
    uint8_t freq1 = spi_read_register(FREQ1);
    uint8_t freq2 = spi_read_register(FREQ2);
    uint32_t freq = ((uint32_t)freq2 << 16) | ((uint32_t)freq1 << 8) | freq0;
    return freq * 396.7285156;  // Convert to Hz
}

// returns MDMCFG4 (contains DRATE_E[3:0]) as first byte and MDMCFG3 (contains DRATE_M[7:0]) as second byte packed together
uint16_t get_dataRateRegs() {
    uint8_t mdmcfg4 = spi_read_register(MDMCFG4); // mdmcfg4 contains DRATE_E[3:0]
    uint8_t mdmcfg3 = spi_read_register(MDMCFG3); // mdmcfg3 contains DRATE_M[7:0]
    printf("MDMCFG4: 0x%02X, MDMCFG3: 0x%02X\n", mdmcfg4, mdmcfg3);

    // pack two bytes into one uint16
    return (mdmcfg4 << 8) | mdmcfg3;
}

// cannot go below 24.80 kbps? turn off manchester encoding, reduce RX filter bandwidth?
void set_dataRate(double kbps) {
    if (kbps < 0.6 || kbps > 500) {
        printf("Error: Data rate %.2f kbps out of range (0.6 - 500 kbps)\n", kbps);
        return;
    }

    // Compute DRATE_E
    double exponent = log2((kbps * pow(2, 20)) / CRYSTAL_FREQUENCY);
    uint8_t DRATE_E = (uint8_t)floor(exponent);  // Take the floor as per the datasheet

    // Compute DRATE_M
    double mantissa = ((kbps * pow(2, 28)) / (CRYSTAL_FREQUENCY * pow(2, DRATE_E))) - 256;
    uint8_t DRATE_M = (uint8_t)round(mantissa);  // Round to the nearest integer

    // Special case: If DRATE_M rounds to 256, adjust DRATE_E and set DRATE_M to 0
    if (DRATE_M > 255) {
        DRATE_E += 1;
        DRATE_M = 0;
    }

    uint8_t mdmcfg4_old = spi_read_register(MDMCFG4);
    uint8_t mdmcfg4_new = (mdmcfg4_old & 0xF0) | (DRATE_E & 0x0F);
    spi_write_register(MDMCFG4, mdmcfg4_new);
    spi_write_register(MDMCFG3, DRATE_M);

    printf("Set Data Rate: %.2f kbps -> DRATE_E = %d, DRATE_M = 0x%02X\n", kbps, DRATE_E, DRATE_M);
}

double get_dataRate() {
    uint8_t mdmcfg4 = spi_read_register(MDMCFG4);  // Read MDMCFG4 (DRATE_E)
    uint8_t mdmcfg3 = spi_read_register(MDMCFG3);  // Read MDMCFG3 (DRATE_M)

    uint8_t DRATE_E = mdmcfg4 & 0x0F;  // Extract lower 4 bits
    uint8_t DRATE_M = mdmcfg3;         // Mantissa is full 8 bits

    double dataRate = ((256 + DRATE_M) * pow(2, DRATE_E) * 26000000.0) / pow(2, 28);

    printf("Current Data Rate: %.2f kbps (DRATE_E = %d, DRATE_M = 0x%02X)\n", dataRate, DRATE_E, DRATE_M);
    
    return dataRate;
}


// void set_output_power_level(int8_t dBm) {
//     uint8_t pa = 0xC0;

//     if      (dBm <= -30) pa = 0x00;
//     else if (dBm <= -20) pa = 0x01;
//     else if (dBm <= -15) pa = 0x02;
//     else if (dBm <= -10) pa = 0x03;
//     else if (dBm <= 0)   pa = 0x04;
//     else if (dBm <= 5)   pa = 0x05;
//     else if (dBm <= 7)   pa = 0x06;
//     else if (dBm <= 10)  pa = 0x07;

//     spi_write_register(FREND0,pa);
// }

// void tx_fifo_erase(uint8_t *txbuffer) {
//     memset(txbuffer, 0, sizeof(FIFOBUFFER));  //erased the TX_fifo array content to "0"
// }

// void rx_fifo_erase(uint8_t *rxbuffer) {
//     memset(rxbuffer, 0, sizeof(FIFOBUFFER)); //erased the RX_fifo array content to "0"
// }

void receive(void) {
    sidle(); // includes 100us sleep            
    spi_write_strobe(SRX);

    uint8_t marcstate = 0xFF;   // unknown/dummy state value
    while (marcstate != 0x0D) { // 0x0D = RX 
        marcstate = (spi_read_register(MARCSTATE) & 0x1F); // get current state
        printf("marcstate_rx: 0x%02X\r\n", marcstate); // see page 93/98 of datasheet
    }
    // delayMicroseconds(100);
}

int16_t rssi_convert(uint8_t rssi_hex, uint8_t offset) {
    int16_t raw = (rssi_hex >= 128) ? ((int16_t)rssi_hex - 256) / 2 : ((int16_t)rssi_hex) / 2;
    return raw - offset;
}

void drawSpectrum(SDL_Renderer* renderer, const std::vector<int16_t>& rssi, double channel_spacing) {
//     // For each pair of adjacent channels (i-1, i),
//     // draw a line from the previous RSSI to the current RSSI.

//     float xScale = SCREEN_WIDTH / (float)NUM_CHANNELS;

//     for (size_t i = 1; i < rssi.size(); i++) {
//         int16_t rssiPrev = rssi[i - 1];
//         int16_t rssiCurr = rssi[i];
        
//         int x1 = (int)((i - 1) * xScale);
//         int x2 = (int)(i * xScale);
        
//         // scale -120..0 => 200..0
//         int y1 = SPECTRUM_HEIGHT - (rssiPrev + 120) * (SPECTRUM_HEIGHT / 120);
//         int y2 = SPECTRUM_HEIGHT - (rssiCurr + 120) * (SPECTRUM_HEIGHT / 120);
        
//         // draw line in green
//         aalineRGBA(renderer, x1, y1, x2, y2, 0, 255, 0, 255);
//     }
    double base_freq_hz = get_frequency(); // Get center frequency in Hz
    double total_bandwidth = channel_spacing * NUM_CHANNELS;
    double start_freq = base_freq_hz - (total_bandwidth / 2);

    for (size_t ch = 1; ch < rssi.size(); ch++) {
        double freq1 = start_freq + (ch - 1) * channel_spacing;
        double freq2 = start_freq + ch * channel_spacing;
    
        int x1 = (int)((freq1 - start_freq) / total_bandwidth * SCREEN_WIDTH);
        int x2 = (int)((freq2 - start_freq) / total_bandwidth * SCREEN_WIDTH);
    
        int y1 = SPECTRUM_HEIGHT - (rssi[ch - 1] + 120) * (SPECTRUM_HEIGHT / 120);
        int y2 = SPECTRUM_HEIGHT - (rssi[ch] + 120) * (SPECTRUM_HEIGHT / 120);
    
        aalineRGBA(renderer, x1, y1, x2, y2, 0, 255, 0, 255);
    }
}



void drawWaterfall(SDL_Renderer* renderer, const std::vector<std::vector<int16_t>>& wf) {
    // We have WATERFALL_ROWS rows, each row is a vector<int16_t> of length NUM_CHANNELS.
    // row 0 is the newest data, row WATERFALL_ROWS-1 is oldest.
    // We'll map that to y from 200..400.

    float colWidth  = SCREEN_WIDTH / (float)NUM_CHANNELS;
    float rowHeight = WATERFALL_HEIGHT / (float)WATERFALL_ROWS;

    // For each row y in [0..WATERFALL_ROWS-1]
    for (int row = 0; row < WATERFALL_ROWS; row++) {
        // row 0 => top line of the waterfall area
        // row 99 => bottom line (if WATERFALL_ROWS=100)
        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            int16_t rssi = wf[row][ch];
            // scale rssi from ~ -100..-20 or so into [0..255].
            // example: saturate it in [-100..-20], map that to 0..255
            // This is just a guess - tweak to taste.
            if (rssi < -100) rssi = -100;
            if (rssi > -20)  rssi = -20;
            int val = (rssi + 100) * (255 / 80); // -100 => 0, -20 => 80 => 255
            
            // color map: let's do simple "blue -> red"
            Uint8 r = (Uint8)val;
            Uint8 g = 0;
            Uint8 b = (Uint8)(255 - val);
            
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);

            SDL_Rect rect;
            rect.x = (int)(ch * colWidth);
            rect.y = (int)(SPECTRUM_HEIGHT + row * rowHeight); // bottom area
            rect.w = (int)(colWidth + 1);  // +1 to avoid gaps
            rect.h = (int)(rowHeight + 1);

            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void drawText(const char *text, int x, int y) {
    SDL_Color textColor = {255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Draw axis labels for frequency (x-axis) and dBm (y-axis)
void drawAxes(double total_bandwidth) {
    double start_freq_mhz = (get_frequency() - total_bandwidth / 2) / 1e6;
    double end_freq_mhz = (get_frequency() + total_bandwidth / 2) / 1e6;
    
    int numXLabels = 5;
    for (int i = 0; i < numXLabels; i++) {
        int x = (int)(i * (SCREEN_WIDTH / (double)(numXLabels - 1)));
        double labelFreq = start_freq_mhz + i * (end_freq_mhz - start_freq_mhz) / (numXLabels - 1);
        
        char freqLabel[32];
        sprintf(freqLabel, "%.2f MHz", labelFreq);
        drawText(freqLabel, x, SPECTRUM_HEIGHT + 5);
    }
    
    int numYLabels = 6;
    for (int i = 0; i < numYLabels; i++) {
        // dBm values: 0, -20, -40, ... , -100
        int dBmValue = 0 - i * 20;
        char dBmLabel[16];
        sprintf(dBmLabel, "%d dBm", dBmValue);
        // Map dBm to y-coordinate using the same scale as in drawSpectrum():
        // y = SPECTRUM_HEIGHT - (rssi + 120) * (SPECTRUM_HEIGHT/120)
        int y = SPECTRUM_HEIGHT - (int)((dBmValue + 120) * (SPECTRUM_HEIGHT / 120.0));
        drawText(dBmLabel, 5, y - 10);
    }
}

int main() {

    // SETUP
    setupSPI();
    setupSDL(&window, &renderer, &font);

    // MODULATION TYPE
    set_modulation_type(3);
    const char* mod_type = get_modulation_type();

    // FREQUENCY
    set_specific_freq(314900000); // 314.9 Mhz
    double freq = get_frequency(); // Hz

    // rssi_values.reserve(10000000);
    // waterfall.reserve(10000);
    
    // MANCHESTER ENCODING
    uint8_t mdmcfg2 = spi_read_register(MDMCFG2);
    if (mdmcfg2 & 0x08) {
        printf("Manchester Encoding is ENABLED\n");
    }

    // CHANNEL SPACING
    // spi_write_register(MDMCFG0, 0x00);                  // MINIMUM CHANNEL SPACING FOR HIGH RES (SETS CHANSPC_M TO 0)
    uint8_t chanspc_m = spi_read_register(MDMCFG0);     // contains CHANSPC_M[7:0] (the whole byte)

    uint8_t mdmcfg1 = spi_read_register(MDMCFG1);       // contains CHANSPC_E[1:0]
    mdmcfg1 = mdmcfg1 & 0xFC;                           // MINIMUM CHANNEL SPACING (SETS CHANSPC_E TO 0)
    // spi_write_register(MDMCFG1, mdmcfg1);

    uint8_t chanspc_e = mdmcfg1 & 0x03;                 // only keep lowest two bits (1:0)

    double channel_spacing = 26000000 / (1 << 18) * (256 + chanspc_m) * (1 << chanspc_e); // see datasheet formula (pg. 78)
    double total_bandwidth = channel_spacing * NUM_CHANNELS;

    
    // // GET DATA RATE
    // uint16_t drateRegs = get_dataRateRegs();
    // uint8_t drate_e = ((drateRegs >> 8) & 0x0F); printf("DRATE_E: 0x%02X = %u\n", drate_e, drate_e);
    // uint8_t drate_m = drateRegs & 0xFF; printf("DRATE_M: 0x%02X = %u\n", drate_m, drate_m);
    
    // get_dataRate();
    
    // CHANNEL BANDWIDTH (SET TO MINIMUM TO ALLOW LOWER kbps?)
    // uint8_t mdmcfg4 = (drateRegs >> 8);
    // mdmcfg4 = (mdmcfg4 & 0x0F) | 0x00; // Set CHANBW_E = 0, CHANBW_M = 0 (Smallest BW)
    // spi_write_register(MDMCFG4, mdmcfg4);
    
    // // SET DATA RATE
    // set_dataRate(10);
    
    // get_dataRate();
    // drateRegs = get_dataRateRegs();
    // drate_e = ((drateRegs >> 8) & 0xFF) & 0x0F; printf("DRATE_E: 0x%02X = %u\n", drate_e, drate_e);
    // drate_m = drateRegs & 0xFF; printf("DRATE_M: 0x%02X = %u\n", drate_m, drate_m);

    print_MDMCFGs();


    char currText[128];
    sprintf(currText, "Mod: %s    Freq: %.1f MHz", mod_type, freq / 1e6);
    printf("Mode: %s, Freq: %.0fHz, ChSpc: %.0fHz, TotalBand: %.0f\n", mod_type, freq, channel_spacing, total_bandwidth);

    receive(); // DO NEED
    usleep(800);
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = false; }
        }

        // scan channels within current freq
        for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
            sidle();                // or spi_write_strobe(SIDLE);
            
            spi_write_register(CHANNR, ch);

            spi_write_strobe(SRX);  //writes receive strobe (receive mode)
            usleep(300);

            // uint8_t marcstate = 0xFF;                     //set unknown/dummy state value
            // while (marcstate != 0x0D) {          //0x0D = RX 
            //     marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of cc1100 to be sure in RX
            //     printf("marcstate_rx: 0x%02X\r\n", marcstate); // see page 93/98 of datasheet
            // }
            
            // get RSSI for current channel
            uint8_t rssi_hex = spi_read_register(RSSI);
            uint8_t offset =  RSSI_OFFSET_315MHZ; // e.g., RSSI_OFFSET_315MHZ aka 64 aka 0x40
            int16_t rssi_dbm = rssi_convert(rssi_hex, offset);
            
            // printf("RSSI: %d dBm, Mode: %s, Freq: %.0fHz, Ch #: %u, ChSpc: %.0fHz\n", rssi_dbm, mod_type, freq, ch, channel_spacing);
            rssi_values[ch] = rssi_dbm;
        }
        
        // move rows forward/down
        for (int i = WATERFALL_ROWS - 1; i > 0; i--) { waterfall[i] = waterfall[i - 1]; }
        // set new top row of data
        waterfall[0] = rssi_values;
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        drawSpectrum(renderer, rssi_values, channel_spacing);
        drawWaterfall(renderer, waterfall);
        drawText(currText, 200, 10);
        drawAxes(total_bandwidth/1e6);
        SDL_RenderPresent(renderer);
        SDL_Delay(50); // 20 FPS
    }
     
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}


// g++ cc1101_drivers.cpp cc1101_config.cpp -o cc1101_driver -I/usr/include/SDL2 -lwiringPi -lSDL2 -lSDL2_ttf -lSDL2_gfx