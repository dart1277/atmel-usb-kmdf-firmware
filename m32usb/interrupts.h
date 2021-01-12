//OSRFX2 vendor requests defines:
#include <avr/io.h>
#define READ_7_SEGMENT 0xd4
#define READ_SWITCHES 0xd6
#define READ_BARGRAPH 0xd7
#define SET_BARGRAPH 0xd8
#define IS_HIGH_SPEED 0xd9
#define SET_7_SEGMENT 0xdb
uint8_t usb_ep6_write(uint8_t *ptr,uint8_t len,void (*callback)());
uint8_t usb_ep4_read(uint8_t *ptr,uint8_t len,void (*callback)());


