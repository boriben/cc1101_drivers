#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "cc1101_config.h"
#include "ansi_colors.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 400

#define SPECTRUM_HEIGHT 200
#define WATERFALL_HEIGHT 200
#define WATERFALL_ROWS 100

#define NUM_CHANNELS 30    // num x values on spectrum (resolution)

std::vector<int16_t> rssi_values(NUM_CHANNELS, -100);
std::vector<std::vector<int16_t>> waterfall(WATERFALL_ROWS, std::vector<int16_t>(NUM_CHANNELS, -100));

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

void spi_write_strobe(uint8_t commandRegister) {
    wiringPiSPIDataRW(0, &commandRegister, 1) ;
}

void spi_write_register(uint8_t spi_instr, uint8_t value) {
    uint8_t tbuf[2] = {0};
    tbuf[0] = spi_instr | WRITE_SINGLE_BYTE;
    tbuf[1] = value;
    wiringPiSPIDataRW(0, tbuf, 2) ;
    return;
}

void spi_write_burst(uint8_t startReg, const uint8_t *data, uint8_t len) {
    uint8_t buff[len + 1];
    buff[0] = startReg | WRITE_BURST;
    for (uint8_t i = 0; i < len ;i++) { buff[i+1] = data[i]; }
    // memcpy(buff + 1, data, len);
    wiringPiSPIDataRW(0, buff, len + 1);
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
    mdmcfg2 = (mdmcfg2 & 0x8F) | (mod_type << 4);   // zero the mod_type bits then set them
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

void set_frequency(double hz) {
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

    uint8_t mdmcfg4 = spi_read_register(MDMCFG4);
    mdmcfg4 = (mdmcfg4 & 0xF0) | DRATE_E;
    spi_write_register(MDMCFG4, mdmcfg4);
    spi_write_register(MDMCFG3, DRATE_M);
}

double get_dataRate() {
    uint8_t mdmcfg4 = spi_read_register(MDMCFG4);  // Read MDMCFG4 (DRATE_E)
    uint8_t mdmcfg3 = spi_read_register(MDMCFG3);  // Read MDMCFG3 (DRATE_M)

    uint8_t DRATE_E = mdmcfg4 & 0x0F;  // Extract lower 4 bits
    uint8_t DRATE_M = mdmcfg3;         // Mantissa is full byte

    double dataRate = ((256 + DRATE_M) * pow(2, DRATE_E) * 26000000.0) / pow(2, 28);
    
    return dataRate;
}

// calculate channel spacing (Hz) from CHANSPC_M and CHANSPC_E
double get_channel_spacing() {
    uint8_t chanspc_regs[2];
    spi_read_burst(MDMCFG1, chanspc_regs, 2);
    
    uint8_t mdmcfg1 = chanspc_regs[0];                  // MDMCFG1 contains CHANSPC_E[1:0] (lowest 2 bits)
    uint8_t chanspc_e = mdmcfg1 & 0x03;                 // mask with 00000011 (0x03) to get lowest 2
    uint8_t chanspc_m = chanspc_regs[1];                // MDMCFG0 contains CHANSPC_M[7:0] (the whole byte)

    return (26000000 / (1 << 18) * (256 + chanspc_m) * (1 << chanspc_e)); // see datasheet formula (pg. 78)
}

// E: 0-3   M: 0-255
// E, M = 0 -> minimum chanspc of ~25 kHz
// E, M = 3, 255 -> maximum chanscp of ~405 kHz
void set_channel_spacing(uint8_t chanspc_e, uint8_t chanspc_m) {
    // MDMCFG1 contains CHANSPC_E[1:0] (lowest 2 bits)
    // MDMCFG0 contains CHANSPC_M[7:0] (the whole byte)

    uint8_t mdmfcg1 = spi_read_register(MDMCFG1);

    mdmfcg1 = (mdmfcg1 & 0xFC) | chanspc_e;

    spi_write_register(MDMCFG1, mdmfcg1);
    spi_write_register(MDMCFG0, chanspc_m);
}

double get_channelBW() {
    uint8_t mdmcfg4 = spi_read_register(MDMCFG4);
    
    // CHANBW_E from [7:6] and CHANBW_M from [5:4]
    uint8_t chanbw_e = (mdmcfg4 >> 6) & 0x03;
    uint8_t chanbw_m = (mdmcfg4 >> 4) & 0x03;

    return CRYSTAL_FREQUENCY / (8 * (4 + chanbw_m) * (1 << chanbw_e));
}

// 0 -> ~812 kHz channel filter BW (max)
// 3 -> ~58 kHz (min)
void set_channelBW(uint8_t bw) {
    uint8_t mdmcfg4 = spi_read_register(MDMCFG4);

    mdmcfg4 = (mdmcfg4 & 0x0F) | (bw << 6) | (bw << 4);   // set the four chanbw bits to zero then set both chanbw_e and chanbw_m to the same 2 bit value
    spi_write_register(MDMCFG4, mdmcfg4);
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

    // setup
    setupSPI();
    setupSDL(&window, &renderer, &font);

    // MODULATION TYPE
    set_modulation_type(3);
    const char* mod_type = get_modulation_type();

    // FREQUENCY
    set_frequency(314000000);
    double freq = get_frequency();
    printf("Freq: %.1f Mhz\n", freq/1e6);

    // CHANNEL SPACING
    // set_channel_spacing(1, 40);
    // double channel_spacing = get_channel_spacing();

    // TOTAL BANDWIDTH
    // double total_bandwidth = channel_spacing * NUM_CHANNELS;
    
    // CHANNEL BANDWIDTH
    // set_channelBW(3);
    // double chanBW = get_channelBW();
    
    // DATA RATE
    // set_dataRate(100);
    // double data_rate = get_dataRate();

    // PACKET CONTROL?
    // spi_write_register(PKTCTRL0, 0x32);


    // print_MDMCFGs();
    
    // printf("Mode: %s, Freq: %.1f MHz, ChSpc: %.1f kHz, TotalBand: %.1f MHz, DataRate: %.1f kbps, ChanBW: %.1f kHz\n", mod_type, freq / 1e6, channel_spacing / 1e3, total_bandwidth / 1e6, data_rate, chanBW / 1e3);

    receive(); // DO NEED

    // SCREEN TEXT
    char currText[128];
    sprintf(currText, "Mod: %s    Freq: %.1f MHz", mod_type, freq / 1e6);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { running = false; }
        }


        for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
            double new_freq = 314000000 + ch * 10000;
            set_frequency(new_freq);
            uint8_t marcstate;
            // do {
            //     marcstate = spi_read_register(MARCSTATE) & 0x1F;
            // } while (marcstate != 0x0D);  // 0x0D = RX Mode
            int maxTries = 1000;
            do {
                marcstate = spi_read_register(MARCSTATE) & 0x1F;
                maxTries--;
                delayMicroseconds(100); 
            } while (marcstate != 0x0D && maxTries > 0);

            if (marcstate != 0x0D) {
                printf("ERROR: Never reached RX state. Current MARCSTATE=0x%02X\n", marcstate);
            }

        
            uint8_t rssi_hex = spi_read_register(RSSI);
            int16_t rssi_dbm = rssi_convert(rssi_hex, RSSI_OFFSET_315MHZ);
        
            // printf("Freq: %.1f MHz, RSSI: %d dBm\n", new_freq / 1e6, rssi_dbm);
            rssi_values[ch] = rssi_dbm;
        }
        
        
        // move rows forward/down
        for (int i = WATERFALL_ROWS - 1; i > 0; i--) { waterfall[i] = waterfall[i - 1]; }
        // set new top row of data
        waterfall[0] = rssi_values;
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        drawSpectrum(renderer, rssi_values, 10000);
        drawWaterfall(renderer, waterfall);
        drawAxes((NUM_CHANNELS-1)*10000);
        drawText(currText, 200, 10);
        SDL_RenderPresent(renderer);
        SDL_Delay(30);
    }
     
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}


// g++ cc1101_drivers.cpp cc1101_config.cpp -o cc1101_driver -I/usr/include/SDL2 -lwiringPi -lSDL2 -lSDL2_ttf -lSDL2_gfx
// git add . && git commit -m "Your commit message" && git push origin main
