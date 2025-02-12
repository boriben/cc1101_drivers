#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  // for usleep()
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <algorithm> // std::max, std::min
#include <cmath>
#include <wiringPi.h>       // pinMode(), digitalWrite(), digitalRead(), wiringPiSetup(), ...
#include <wiringPiSPI.h>    // wiringPiSPISetup(), wiringPiSPIDataRW(), ... 
#include "cc1101_config.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 400

#define SPECTRUM_HEIGHT 200

#define WATERFALL_HEIGHT 200
#define WATERFALL_ROWS 100

#define NUM_CHANNELS 100     // num x values on spectrum (resolution)

std::vector<int16_t> rssi_values(NUM_CHANNELS, -100);
std::vector<std::vector<int16_t>> waterfall(WATERFALL_ROWS, std::vector<int16_t>(NUM_CHANNELS, -100));

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

void spi_write_strobe(uint8_t spi_instr) {
    uint8_t tbuf[1] = {0};
    tbuf[0] = spi_instr;
    //printf("SPI_data: 0x%02X\n", tbuf[0]);
    wiringPiSPIDataRW (0, tbuf, 1) ;
}

void spi_write_register(uint8_t spi_instr, uint8_t value) {
    uint8_t tbuf[2] = {0};
    tbuf[0] = spi_instr | WRITE_SINGLE_BYTE;
    tbuf[1] = value;
    uint8_t len = 2;
    wiringPiSPIDataRW (0, tbuf, len) ;
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
     wiringPiSPIDataRW (0, tbuf, len + 1) ;
}

uint8_t spi_read_register(uint8_t spi_instr) {
    uint8_t value;
    uint8_t rbuf[2] = {0};
    rbuf[0] = spi_instr | READ_SINGLE_BYTE;
    uint8_t len = 2;
    wiringPiSPIDataRW (0, rbuf, len) ;
    value = rbuf[1];
    //printf("SPI_arr_0: 0x%02X\n", rbuf[0]);
    //printf("SPI_arr_1: 0x%02X\n", rbuf[1]);
    return value;
}

void spi_read_burst(uint8_t spi_instr, uint8_t *pArr, uint8_t len) {
    uint8_t rbuf[len + 1];
    rbuf[0] = spi_instr | READ_BURST;   // add read burst mask bit to front of instruction
    wiringPiSPIDataRW(0, rbuf, len + 1);    // read data into buffer
    for (uint8_t i = 0; i < len; i++) { pArr[i] = rbuf[i+1]; } // copy buffer to our array
}

void sidle(void) {
    spi_write_strobe(SIDLE);
    delayMicroseconds(100);
}

// void set_mode(uint8_t mode) {
//     switch (mode) {
//         case 0x01:
//                     spi_write_burst(WRITE_BURST,cc1100_GFSK_1_2_kb,CFG_REGISTER);
//                     break;
//         case 0x02:
//                     spi_write_burst(WRITE_BURST,cc1100_GFSK_38_4_kb,CFG_REGISTER);
//                     break;
//         case 0x03:
//                     spi_write_burst(WRITE_BURST,cc1100_GFSK_100_kb,CFG_REGISTER);
//                     break;
//         case 0x04:
//                     spi_write_burst(WRITE_BURST,cc1100_MSK_250_kb,CFG_REGISTER);
//                     break;
//         case 0x05:
//                     spi_write_burst(WRITE_BURST,cc1100_MSK_500_kb,CFG_REGISTER);
//                     break;
//         case 0x06:
//                     spi_write_burst(WRITE_BURST,cc1100_OOK_4_8_kb,CFG_REGISTER);
//                     break;
//     }
//     return;
// }

// a second set mode function?
void set_modulation_type(uint8_t cfg) {
    uint8_t data;
    data = spi_read_register(MDMCFG2);
    data = (data & 0x8F) | (((cfg) << 4) & 0x70);
    spi_write_register(MDMCFG2, data);
    //printf("MDMCFG2: 0x%02X\n", data);
}

const char* get_modulation_type(uint8_t mod_cfg) {
    switch ((mod_cfg >> 4) & 0x07) { // get mod type bits [6:4]? from MDMCFG2 register
        case 1: return "GFSK_1.2kb";
        case 2: return "GFSK_38.4kb";
        case 3: return "GFSK_100kb";
        case 4: return "MSK_250kb";
        case 5: return "MSK_500kb";
        case 6: return "OOK_4.8kb";
        default: return "wtf idk";
    }
}



void set_ISM(uint8_t ism_freq) {
    uint8_t freq2, freq1, freq0;

    switch (ism_freq) {
        case 0x01:                                                          //315MHz
                    freq2=0x0C;
                    freq1=0x1D;
                    freq0=0x89;
                    spi_write_burst(PATABLE_BURST, patable_power_315, 8);
                    break;
        case 0x02:                                                          //433.92MHz
                    freq2=0x10;
                    freq1=0xB0;
                    freq0=0x71;
                    spi_write_burst(PATABLE_BURST, patable_power_433, 8);
                    break;
        case 0x03:                                                          //868.3MHz
                    freq2=0x21;
                    freq1=0x65;
                    freq0=0x6A;
                    spi_write_burst(PATABLE_BURST, patable_power_868, 8);
                    break;
        case 0x04:                                                          //915MHz
                    freq2=0x23;
                    freq1=0x31;
                    freq0=0x3B;
                    spi_write_burst(PATABLE_BURST, patable_power_915, 8);
                    break;
    }

    spi_write_register(FREQ2,freq2);                                         //stores the new freq setting for defined ISM band
    spi_write_register(FREQ1,freq1);
    spi_write_register(FREQ0,freq0);

    return;
}

uint32_t get_frequency() {
    uint8_t freq0 = spi_read_register(FREQ0);
    uint8_t freq1 = spi_read_register(FREQ1);
    uint8_t freq2 = spi_read_register(FREQ2);
    uint32_t freq = ((uint32_t)freq2 << 16) | ((uint32_t)freq1 << 8) | freq0;
    return freq * 396.7285156;  // Convert to Hz
}

void set_channel(uint8_t channel){
    spi_write_register(CHANNR,channel);
    return;
}

void set_patable(uint8_t *patable_arr) {
    spi_write_burst(PATABLE_BURST,patable_arr,8);   //writes output power settings to cc1100    "104us"
}

void set_output_power_level(int8_t dBm) {
    uint8_t pa = 0xC0;

    if      (dBm <= -30) pa = 0x00;
    else if (dBm <= -20) pa = 0x01;
    else if (dBm <= -15) pa = 0x02;
    else if (dBm <= -10) pa = 0x03;
    else if (dBm <= 0)   pa = 0x04;
    else if (dBm <= 5)   pa = 0x05;
    else if (dBm <= 7)   pa = 0x06;
    else if (dBm <= 10)  pa = 0x07;

    spi_write_register(FREND0,pa);
}

void tx_fifo_erase(uint8_t *txbuffer) {
    memset(txbuffer, 0, sizeof(FIFOBUFFER));  //erased the TX_fifo array content to "0"
}

void rx_fifo_erase(uint8_t *rxbuffer) {
    memset(rxbuffer, 0, sizeof(FIFOBUFFER)); //erased the RX_fifo array content to "0"
}

void receive(void) {
    sidle();                              //sets to idle first.
    spi_write_strobe(SRX);                //writes receive strobe (receive mode)

    uint8_t marcstate = 0xFF;                     //set unknown/dummy state value
    while (marcstate != 0x0D) {          //0x0D = RX 
        marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of cc1100 to be sure in RX
        printf("marcstate_rx: 0x%02X\r\n", marcstate); // see page 93/98 of datasheet
    }
    //printf("\r\n");

    // delayMicroseconds(100);
}

int16_t rssi_convert(uint8_t rssi_hex, uint8_t offset) {
    int16_t raw = (rssi_hex >= 128) ? ((int16_t)rssi_hex - 256) / 2 : ((int16_t)rssi_hex) / 2;
    return raw - offset;
}

void drawSpectrum(SDL_Renderer* renderer, const std::vector<int16_t>& rssi)
{
    // For each pair of adjacent channels (i-1, i),
    // draw a line from the previous RSSI to the current RSSI.

    // We assume the top half of the screen is y=0..SPECTRUM_HEIGHT
    // We'll scale x from 0..SCREEN_WIDTH, y so that:
    //   -120 dBm -> near bottom (y=200)
    //    0 dBm   -> near top    (y=0)
    // Multiply dBm by ~2 just for a visible scale.
    
    float xScale = SCREEN_WIDTH / (float)NUM_CHANNELS;
    
    // We can use SDL2_gfx's aalineRGBA (antialiased line) or thickLineRGBA
    // We'll use green lines.
    for (size_t i = 1; i < rssi.size(); i++) {
        int16_t rssiPrev = rssi[i - 1];
        int16_t rssiCurr = rssi[i];
        
        // clamp to [ -120 .. 0 ] just to avoid weird extremes
        if (rssiPrev < -120) rssiPrev = -120;
        if (rssiCurr < -120) rssiCurr = -120;
        if (rssiPrev > 0)    rssiPrev = 0;
        if (rssiCurr > 0)    rssiCurr = 0;
        
        int x1 = (int)((i - 1) * xScale);
        int x2 = (int)(i * xScale);
        
        // scale -120..0 => 200..0
        int y1 = SPECTRUM_HEIGHT - (rssiPrev + 120) * (SPECTRUM_HEIGHT / 120);
        int y2 = SPECTRUM_HEIGHT - (rssiCurr + 120) * (SPECTRUM_HEIGHT / 120);
        
        // draw line in green
        aalineRGBA(renderer, x1, y1, x2, y2, 0, 255, 0, 255);
    }
}

void drawWaterfall(SDL_Renderer* renderer, 
                   const std::vector<std::vector<int16_t>>& wf)
{
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

// assumes renderer has been defined externally/globally
// void drawButton(int x, int y, int w, int h, TTF_Font *font, SDL_Renderer* renderer) {
//     SDL_Rect button = {x, y, w, h};
//     SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);  // Blue
//     SDL_RenderFillRect(renderer, &button);
//     renderText("bootun", x, y, font, renderer);
// }

bool buttonClicked(int clickX, int clickY, int buttonX, int buttonY, int buttonW, int buttonH) {
    if (clickX >= buttonX && clickX <= buttonX+buttonW && clickY >= buttonY && clickY <= buttonY+buttonH)
        return true;
    return false;
}

void clearSpectrumArea() {
    SDL_Rect spectrumRect = {0, 0, SCREEN_WIDTH, SPECTRUM_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &spectrumRect);
}

int main() {

    wiringPiSetup();

    pinMode(GDO2, INPUT);
    pinMode(SS_PIN, OUTPUT);

    // 4MHz SPI speed
    if (wiringPiSPISetup(0, 8000000) < 0) { printf ("ERROR: wiringPiSPISetup failed!\r\n"); }

    // reset
    // digitalWrite(SS_PIN, LOW);
    // delayMicroseconds(10);
    // digitalWrite(SS_PIN, HIGH);
    // delayMicroseconds(40);
    // spi_write_strobe(SRES);
    // delay(1);

    spi_write_strobe(SFTX); delayMicroseconds(100);	//flush the TX_fifo content
    spi_write_strobe(SFRX); delayMicroseconds(100);	//flush the RX_fifo content

    uint8_t partnum, version;
    partnum = spi_read_register(PARTNUM); //reads CC1100 partnumber
    version = spi_read_register(VERSION); //reads CC1100 version number
    printf("Partnumber: 0x%02X\r\n", partnum);
    printf("Version   : 0x%02X\r\n", version);

    // set_mode(6);   // doesn't work?
    set_modulation_type(6);
    set_ISM(1);      // 315Mhz

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    // srand(time(0));
    window = SDL_CreateWindow("spectrum analyzer",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
    if (!font) { printf("failed to load font\n"); return 1;}

    // rssi_values.reserve(10000000);
    // waterfall.reserve(10000);

    // get current mod type and freq (debugging and text)
    uint8_t mdmcfg2 = spi_read_register(MDMCFG2);
    const char* mod_type = get_modulation_type(mdmcfg2);
    double freq = get_frequency(); // Hz

    uint8_t chanspc_m = spi_read_register(MDMCFG0);   // contains CHANSPC_M[7:0] (the whole byte)

    uint8_t mdmcfg1 = spi_read_register(MDMCFG1);   // contains CHANSPC_E[1:0]
    uint8_t chanspc_e = mdmcfg1 & 0x03;             // only keep lowest two bits (1:0)

    double channel_spacing = 26000000 / (1 << 18) * (256 + chanspc_m) * (1 << chanspc_e); // see datasheet formula (pg. 78)
    
    char currText[128];
    sprintf(currText, "Mod: %s    Freq: %.3f MHz", mod_type, freq / 1e6);

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
            usleep(500);

            // uint8_t marcstate = 0xFF;                     //set unknown/dummy state value
            // while (marcstate != 0x0D) {          //0x0D = RX 
            //     marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of cc1100 to be sure in RX
            //     printf("marcstate_rx: 0x%02X\r\n", marcstate); // see page 93/98 of datasheet
            // }
            
            // get RSSI for current channel
            uint8_t rssi_hex = spi_read_register(RSSI);
            uint8_t offset =  RSSI_OFFSET_315MHZ; // e.g., RSSI_OFFSET_315MHZ aka 64 aka 0x40
            int16_t rssi_dbm = rssi_convert(rssi_hex, offset);
            
            printf("RSSI: %d dBm, Mode: %s, Freq: %.0fHz, Ch #: %u, ChSpc: %.0fHz\n", rssi_dbm, mod_type, freq, ch, channel_spacing);
            rssi_values[ch] = rssi_dbm;
        }
        
        // move rows forward/down
        for (int i = WATERFALL_ROWS - 1; i > 0; i--) { waterfall[i] = waterfall[i - 1]; }
        // set new top row of data
        waterfall[0] = rssi_values;
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        drawSpectrum(renderer, rssi_values);
        drawWaterfall(renderer, waterfall);
        drawText(currText, 10, 10);
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