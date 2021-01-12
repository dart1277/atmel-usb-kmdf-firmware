#include <avr/io.h>
#include <avr/interrupt.h>
#include<avr/wdt.h>

//UENUM,UEDATX,UEINT are always used directly

void ack_nakio_stall(uint8_t ep);
void ack_fifo(uint8_t ep);
void ack_stp(uint8_t ep);
void ack_out(uint8_t ep);
void ack_in(uint8_t ep);
void reset_ep(uint8_t ep);
void stall_on(uint8_t ep);
void stall_off(uint8_t ep);
void clear_toggle(uint8_t ep);
void killbk(uint8_t ep);
uint8_t ctrldir();
void adden();

void usb_init_hw();
void configure_ep(uint8_t ep);
