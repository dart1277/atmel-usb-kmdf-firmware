/*
 * m32usb.c
 *
 * Created: 3/7/2014 5:45:56 PM
 *  Author: Kornel
 */ 

#include "m32usb.h"
#include "phy_io.h"
#define  TAB_SZ 128
uint8_t tab_r[TAB_SZ];
uint8_t tab_w[TAB_SZ];
static uint8_t volatile read_lock=0;
static uint8_t volatile write_lock=0;
static void callback_read()
{
	read_lock=0;
}

static void callback_write()
{
	write_lock=0;
}

int main(void)
{	uint8_t i;
	DDRC|=LED1;
	usb_init_hw();
    while(1)
    {
        //TODO:: Please write your application code 

		/*PORTC|=LED1;
		_delay_ms(500);
		PORTC&=~LED1;*/
		//_delay_ms(500);
		PINC=LED1;
		
		read_lock=usb_ep4_read(tab_r,TAB_SZ,callback_read);
		if(read_lock==1){
			while(read_lock==1)asm volatile("nop \n\t");//sleep
			for(i=0;i<TAB_SZ;i++)tab_w[i]=tab_r[i];
			write_lock=usb_ep6_write(tab_w,TAB_SZ,callback_write);
			while(write_lock==1)asm volatile("nop \n\t");//sleep
			}
	}
}