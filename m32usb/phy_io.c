#include "phy_io.h"

static void configure_ep0();
static void configure_ep1();
static void configure_ep4();
static void configure_ep6();

//TODO add old UENUM save/restore capability
//to all functions here
 

void ack_nakio_stall(uint8_t ep)
{
	UENUM=ep;
	UEINTX&=0xAD;
}

void ack_fifo(uint8_t ep)
{
	UENUM=ep;
	UEINTX&=0x7F;
}

void ack_stp(uint8_t ep)
{
	UENUM=ep;
	UEINTX&=0xF7;
}

void ack_out(uint8_t ep)
{
	UENUM=ep;
	UEINTX&=0xFB;
}

void ack_in(uint8_t ep)
{
	UENUM=ep;
	UEINTX&=0xFE;
}

void reset_ep(uint8_t ep)
{
	UENUM=ep;
	UERST|=(1<<ep);
	asm volatile("nop");//is this enough?? specs doesn't say anything
	UERST&=0x00;
	configure_ep(ep);
}

void stall_on(uint8_t ep)
{	
	UENUM=ep;
	UECONX|=0x20;
}

void clear_toggle(uint8_t ep)
{
	UENUM=ep;
	UECONX|=0x08;
}

void stall_off(uint8_t ep)
{
	UENUM=ep;
	UECONX|=0x10;
	reset_ep(ep);
}




void killbk(uint8_t ep)
{	
	UENUM=ep;
	UEIENX&=0xFE;
	while((UESTA0X&0x03)!=0x00)//nbusybk!=0?
	{
		UEINTX|=0x04;
		while(UEINTX&0x04)asm volatile("nop\n\t");
	}
	
	UEIENX|=0x01;
}

uint8_t ctrldir() //returns 0 for out xor 4 for in
{
	UENUM=0x00;
	return UESTA1X&0x04;
}

void adden(){
	
	UDADDR|=0x80;
}

void configure_ep(uint8_t ep)
{
	switch(ep)
	{
		case 0:
		configure_ep0();
		break;
		case 1:
		configure_ep1();
		break;
		case 4:
		configure_ep4();
		break;
		case 6:
		configure_ep6();
		break;
	}
	
}

static void configure_ep0() //control ep 8B
{
	UENUM=0x00;//ep num 0
	//clear_toggle(0);
	UECONX=0x01;//enable ep
	UECFG0X=0x00;//control ep
	UECFG1X=0b00000010;//alloc ep
	while((UESTA0X&0x80)^0x80) asm volatile("nop\n\t"); //wait for CFGOK bit
	UEIENX=0b00001101; //unmask setup, out, in interrupt TODO
	ack_in(0);
	
}




static void configure_ep1() //interrupt in ep 8B
{
	UENUM=0x01;//ep num 1
	//UECONX=0x01;//enable ep TODO
	UECFG0X=0b11000001;//interrupt in ep
	UECFG1X=0b00000010;
	while((UESTA0X&0x80)^0x80) asm volatile("nop\n\t"); //wait for CFGOK bit
	UEIENX=(1<<TXINE); //unmask in interrupt
	asm volatile("nop\n\t");
	/*ack_in(1); //we will automatically read data from osrfx2 context
	ack_fifo(1);*/
	
}

static void configure_ep4()// bulk out ep 64B
{
	UENUM=0x04;//ep num 4
	UECONX=0x01;//enable ep TODO
	UECFG0X=0b10000000;//bulk out ep
	UECFG1X=0b00110010;
	while((UESTA0X&0x80)^0x80) asm volatile("nop\n\t"); //wait for CFGOK bit
	UEIENX=0x04;//unmask out interrupt
	}

static void configure_ep6()//bulk in ep 64B
{
	UENUM=0x06;//ep num 6
	UECONX=0x01;//enable ep TODO
	UECFG0X=0b10000001;//bulk in ep
	UECFG1X=0b00110010;
	while((UESTA0X&0x80)^0x80) asm volatile("nop\n\t"); //wait for CFGOK bit
	UEIENX=0x01; //unmask in interrupt
	ack_in(6);
	//ack_fifo(6);TODO !!! MAYBE this causes ep6 to return ZLP at first transfer
}

void usb_init_hw()
{	
	cli();
	//disable watchdog
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	//disable OTG pad
	USBCON &= ~(1 << OTGPADE);
	//power on regulator
	UHWCON|=0x01;
	//disable power detect interrupt
	USBCON &= ~(1 << VBUSTE);
	//clear all interrupts
	UDIEN   = 0;
	USBINT = 0;
	UDINT  = 0;
	//reset USB controller
	USBCON &= ~(1 << USBE);

	//disble pll
	PLLCSR = 0;
	//choose full-speed mode
	UDCON &= ~(1 << LSM);
	//configure pll
	PLLFRQ&=0b11111011;//clear reg to 0
	PLLFRQ|=0b01001010;//pll set to 96MHZ with div by 2 to 48MHz
	//enable pll
	PLLCSR|=0b00010010; //enable pll with 16MHZ clk div by 2
	//chk pll lock
	while((PLLCSR&0x01) ^ 0x01) asm volatile("nop \n\t");
	//enable usb interface

	USBCON |=  (1 << USBE);
		USBCON &= ~(1 << FRZCLK);

	//configure usb (speed=FS default, endpoints)
	//ORDER OF CONFIGURATION MUST!! BE ASCENDING
	configure_ep(0);
	configure_ep(1);
	configure_ep(4);
	configure_ep(6);
	//enable interrupts
	UDIEN|=(1<<EORSTE)|(1<<SOFE)|(1<<SUSPE);
	//wait for usb vbus
	//attach controller
	UDCON  &= ~(1 << DETACH); //DETACH=0
	USBCON |=  (1 << OTGPADE); 
	while((USBSTA&0x01) ^ 0x01) asm volatile("nop \n\t");
	USBINT&=~(1<<VBUSTI);
	sei();
	
}


