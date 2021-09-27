#include <Wire.h>

/*
 * Fundementals 
 */
 
#define I2C_ADDR 0x3c

#define CMD 0x0
#define DATA 0x40

#define SET_CHARGE_PUMP 0x8d
#define ENABLE_CHARGE_PUMP 0x14

// A[7:0]
#define SET_DISP_CONTRAST 0x81 

#define DISP_ON_RAM_FOLLOW 0xa4
#define DISP_ON_RAM_IGNORE 0xa5

#define DISP_NORMAL 0xa6
#define DISP_INVERSE 0xa7

#define DISP_OFF 0xae
#define DISP_ON 0xaf

#define RAM_WIDTH 128
#define RAM_HEIGHT 64

/*
 * Scrolling
 */

// A[7:0] set to 0
// B[2:0] start page addr: 000b(page0) - 111b(page7)
// C[2:0] time interval between ea. scroll step
// D[2:0] end page addr: same as B
// E[7:0] set to 0
// F[7:0] set to ff
#define SET_RIGHT_HORIZ_SCROLL 0x26 
#define SET_LEFT_HORIZ_SCROLL 0x27

// A[7:0] set to 0
// B[2:0] start page addr: 000b(page0) - 111b(page7)
// C[2:0] time interval between ea. scroll step
// D[2:0] end page addr: same as B
// E[7:0] vertical scrolling offset eg. E[5:0] = 0x01 = 1 row, E[5:0] = 0x3f = 63 rows
#define SET_RIGHT_VERT_SCROLL 0x29
#define SET_LEFT_VERT_SCROLL 0x2a

// call after 4 above cmds
#define SCROLL_STOP 0x2e
#define SCROLL_START 0x2f

// A[5:0] set no. rows in top fixed area
// B[6:0] set no. rows in scroll area
#define SET_VERT_SCROLL_AREA 0xa3

/*
 * Addressing
 */

#define SET_ADDR_MODE 0x20
#define HORIZ_MODE 0x0
#define VERT_MODE 0x1
#define PAGE_MODE 0x2

// A[6:0] col start addr 0-127d
// B[6:0] col end addr 0-127d
#define SET_COL_ADDR 0x21

#define COL_LO_START 0x0
#define COL_LO_END 0x0f
#define COL_HI_START 0x10
#define COL_HI_END 0x1f

// A[2:0] page start addr 0-7d
// B[2:0] page end addr 0-7d
#define SET_PAGE_ADDR 0x22

#define PAGE_START 0xb0
#define PAGE_END 0xb7

#define PAGE_LENGTH 8
#define PAGE_WIDTH 128
#define PAGE_LINE 8

/*
 * Hardware configuration
 */

#define SET_SEG_REMAP_HI 0xa0
#define SET_SEG_REMAP_LO 0xa1

// A[5:0] from 16MUX to 64MUX, RESET, 0-14 invalid
#define SET_MUX_RATIO 0xa8

#define SET_COM_OUT_SCAN_DETECTION 0xc0
#define SET_COM_OUT_SCAN_DETECTION_INV 0xc8

// A[5:0] set vertical shift by COM from 0d-63d, reset @ 64d
#define SET_DISPLAY_OFFSET 0xd3

// A[5:4] A[4]=0b sequential, 1b(RESET) alt configuration, 
// .      A[5]=0b(RESET) disable left/right remap, 1b enable
#define SET_COM_CONFIG 0xda

/*
 * Timing & driving
 */
 
// A[3:0] = divide ratio - 1, RESET = 0000b
// A[7:4] set F_OSC, RESET 1000b
#define SET_CLK_DIV 0xd5
#define SET_OSCILLATOR_FREQ 0xd5

// A[3:0] phase 1 period up to 15 DCLK, RESET=2h
// A[7:4] phase 2 period up to 15 DCLK, RESET=2h
#define SET_PRE_CHARGE_PERIOD 0xd9

// A[6:4] 00h - ~0.65Vcc, ~0.77Vcc, ~0.83Vcc
#define SET_VCOMH_DESEL 0xdb 

#define NOP 0xe3


const byte CMD_DEFAULT_INIT[26] = {
                                  DISP_OFF,
                                  SET_OSCILLATOR_FREQ, 0x80, 
                                  SET_MUX_RATIO, 0x3f,
                                  SET_DISPLAY_OFFSET, 0x00, 0x40,
                                  SET_CHARGE_PUMP, ENABLE_CHARGE_PUMP,
                                  SET_ADDR_MODE, HORIZ_MODE,
                                  SET_SEG_REMAP_LO,
                                  SET_COM_OUT_SCAN_DETECTION_INV,
                                  SET_COM_CONFIG, 0x12,
                                  SET_DISP_CONTRAST, 0x80,
                                  SET_PRE_CHARGE_PERIOD, 0xf1,
                                  SET_VCOMH_DESEL, 0x20,
                                  DISP_ON_RAM_FOLLOW, 
                                  DISP_NORMAL,
                                  SCROLL_STOP,
                                  DISP_ON
};

typedef byte *settings_t;

struct _disp_t {
  byte cursor_row, cursor_col;
  settings_t set;
  byte set_len;
  byte current[128];
};

typedef struct _disp_t disp_t;

static void etirw(byte sel, byte data);
static void fill_disp(byte fill);
void update_cursor(disp_t disp, byte new_row, byte new_col);
void set_disp_settings(settings_t set, byte len);
                                  
void setup() {
  Wire.begin();

  disp_t disp;
  disp.set = (settings_t) malloc(sizeof(settings_t) * 26);
  disp.set_len = 26;

  
  memcpy(disp.set, CMD_DEFAULT_INIT, disp.set_len);
  
  set_disp_settings(disp.set, disp.set_len);

  fill_disp(0x99);

  
  
}

void loop() {

}

void set_disp_settings(settings_t set, byte len){
  for(byte i=0; i<len; i++){
    etirw(CMD, *set++);
  }
}


void update_cursor(disp_t *disp, byte new_row, byte new_col){
  
  if(new_row > PAGE_LINE || new_col > PAGE_WIDTH) return;

  disp->cursor_row = new_row;
  disp->cursor_col = new_col;

  etirw(CMD, SET_PAGE_ADDR);
  etirw(CMD, new_row);
  etirw(CMD, PAGE_LINE-1);

  etirw(CMD, SET_COL_ADDR);
  etirw(CMD, new_col);
  etirw(CMD, PAGE_WIDTH-1);
}

static void etirw(byte sel, byte data){
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(sel);
  Wire.write(data);
  Wire.endTransmission();
}

static void fill_disp(byte fill){
  for(int i=0; i < PAGE_WIDTH*8; i++){
    etirw(DATA, fill);
  }
}
