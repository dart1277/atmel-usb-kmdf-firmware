#include "interrupts.h"
#include "phy_io.h"
#include "descriptors.h"
#define EP_0_DATA_LEN 8
#define MAX_CONTROL_TRANSFER_LENGTH 128
enum state {DEF,ADDRESS,CONFIGURED};
enum data_dir {IN,OUT,NONE,ADDR};
static void inline usb_send_status_to_host();
static void usb_process_in_setup();

static uint8_t buffer[MAX_CONTROL_TRANSFER_LENGTH];
static uint8_t ep1_buffer;

static struct
{	
	uint8_t switches;
	uint8_t bargraph;
	uint8_t sev_segment;
	uint8_t speed;
	}osrfx2={.switches=0,.bargraph=0,.sev_segment=0,.speed=0};

static struct
{	
	uint8_t const ep_data_len;
	uint8_t ctrl_tr[EP_0_DATA_LEN];
	enum state state;
	enum data_dir dir;
	uint8_t *buff_ptr;
	uint8_t bytes_left;
	uint16_t transfer_size;
	uint8_t remote_wakeup;
	uint8_t self_powered;
	}context={.state=DEF,.dir=NONE,.buff_ptr=buffer,.bytes_left=0,.transfer_size=0,.ep_data_len=EP_0_DATA_LEN,.remote_wakeup=0,.self_powered=0};

static struct  
{
	uint8_t halt;
	uint8_t *buff_ptr;
	uint8_t bytes_left;
	uint16_t transfer_size;
	uint8_t const ep_data_len;
	void (*callback)();
	
}ep1_context={.halt=0,.buff_ptr=&ep1_buffer,.bytes_left=0,.transfer_size=0,.ep_data_len=8,.callback=0};
static struct
{
	uint8_t halt;
	uint8_t *buff_ptr;
	uint8_t bytes_left;
	uint16_t transfer_size;
	uint8_t const ep_data_len;
	void (*callback)();
}ep4_context={.halt=0,.buff_ptr=0,.bytes_left=0,.transfer_size=0,.ep_data_len=64,.callback=0};
static struct
{
	uint8_t halt;
	uint8_t *buff_ptr;
	uint8_t bytes_left;
	uint16_t transfer_size;
	uint8_t const ep_data_len;
	void (*callback)();
}ep6_context={.halt=0,.buff_ptr=0,.bytes_left=0,.transfer_size=0,.ep_data_len=64,.callback=0};

static uint8_t usb_read_descriptor(uint8_t* desc_ptr,uint8_t offset,uint8_t len)
{
	uint8_t i;
	for(i=0;i<len;i++)
	{
		context.buff_ptr[offset+i]=pgm_read_byte(desc_ptr+i);
	}
	return offset+i;
}

static inline void usb_get_status(usb_dev_reqest_t* req)
{	uint8_t reply;
	if(context.state==DEF){stall_on(0);return;}
	else if(context.state==ADDRESS)
	{
		if((req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE || req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)&&req->wIndex.word==0)
		{
			if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE)
			{
				reply=((context.remote_wakeup&0xFE)<<1|(context.self_powered&0xFE));
				UEDATX=reply;
				UEDATX=0;
			}
			if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)
			{
			
				UEDATX=0;
				UEDATX=0;
			}
			
		}else{stall_on(0);return;}
		
	}
	
	else if(context.state==CONFIGURED)
	{
		if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE )
		{
			reply=((context.remote_wakeup&0xFE)<<1|(context.self_powered&0xFE));
			UEDATX=reply;
			UEDATX=0;
			
		}
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP){ //add here extra endpoints if needed
				if (req->wIndex.word==0)
				{
					UEDATX=0;
					UEDATX=0;
				}
				else if (req->wIndex.word==1)
				{
					UEDATX=ep1_context.halt&0xFE;
					UEDATX=0;
				}
				else if (req->wIndex.word==4)
				{
					UEDATX=ep4_context.halt&0xFE;
					UEDATX=0;
				}
				else if (req->wIndex.word==6)
				{
					UEDATX=ep6_context.halt&0xFE;
					UEDATX=0;
				}else{stall_on(0);return;}
			
		}
		
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_IF && req->wIndex.word==0){
			UEDATX=0;
			UEDATX=0;
		}else{stall_on(0);return;}
	}
context.dir=IN;
context.bytes_left=0;
ack_in(0);
}

static void usb_clear_feature(usb_dev_reqest_t* req)
{
if(context.state==DEF){return;}//TODO maybe stall is required here

else if(context.state==ADDRESS)
{
	if(req->wIndex.word==0)
	{
		if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE)
		{
			if(req->wValue==DEVICE_REMOTE_WAKEUP)context.remote_wakeup=0;
			else{stall_on(0);return;}
		}
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_IF)
		{
			//empty = no interface features available
		}
		
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)
		{
			if(req->wValue==ENDPOINT_HALT){stall_on(0);return;}
		}
		
		
	}else{stall_on(0);return;}


}

else if(context.state==CONFIGURED)
{
	
		if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE)
		{
			if(req->wValue==DEVICE_REMOTE_WAKEUP)context.remote_wakeup=0;
			else{stall_on(0);return;}
		}
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_IF)
		{
			//empty = no interface features available
		}
		
		else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)
		{
			if(req->wValue!=ENDPOINT_HALT ||(req->wValue==ENDPOINT_HALT && req->wIndex.bits.Endpoint==0)||(req->wIndex.bits.Endpoint!=1&&req->wIndex.bits.Endpoint!=4&&req->wIndex.bits.Endpoint!=6)){stall_on(0);return;}
			if(req->wIndex.bits.Endpoint==1){ep1_context.halt=0;killbk(1);reset_ep(1);clear_toggle(1);}
			if(req->wIndex.bits.Endpoint==4){ep1_context.halt=0;reset_ep(4);clear_toggle(4);}//out ep no need to killbk
			if(req->wIndex.bits.Endpoint==6){ep1_context.halt=0;killbk(6);reset_ep(6);clear_toggle(6);}
		}
		
		
	


	}

context.dir=OUT;
usb_send_status_to_host();
}

static void usb_set_feature(usb_dev_reqest_t* req)
{
	if(context.state==DEF){return;}//TODO maybe stall is required here

	else if(context.state==ADDRESS)
	{
		if(req->wIndex.word==0)
		{
			if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE)
			{
				if(req->wValue==DEVICE_REMOTE_WAKEUP)context.remote_wakeup=1;
				else{stall_on(0);return;}
			}
			else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_IF)
			{
				//empty = no interface features available
			}
			
			else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)
			{
				if(req->wValue==ENDPOINT_HALT){stall_on(0);return;}
			}
			
			
			}else{stall_on(0);return;}


		}

		else if(context.state==CONFIGURED)
		{
			
			if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_DE)
			{
				if(req->wValue==DEVICE_REMOTE_WAKEUP)context.remote_wakeup=1;
				else{stall_on(0);return;}
			}
			else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_IF)
			{
				//empty = no interface features available
			}
			
			else if(req->bmRequestType.bits.Recipient==REQ_RECIPIENT_EP)
			{
				if(req->wValue!=ENDPOINT_HALT ||(req->wValue==ENDPOINT_HALT && req->wIndex.bits.Endpoint==0)||(req->wIndex.bits.Endpoint!=1&&req->wIndex.bits.Endpoint!=4&&req->wIndex.bits.Endpoint!=6)){stall_on(0);return;}
				if(req->wIndex.bits.Endpoint==1){ep1_context.halt=1;stall_on(1);}
				if(req->wIndex.bits.Endpoint==4){ep1_context.halt=1;stall_on(4);}
				if(req->wIndex.bits.Endpoint==6){ep1_context.halt=1;stall_on(6);}
			}
			
			
			


		}

		context.dir=OUT;
		usb_send_status_to_host();
	
	
}

static void usb_set_address(usb_dev_reqest_t* req)
{	
	uint8_t add;
	add=(uint8_t)req->wValue;
	UDADDR=add;
	context.dir=ADDR;
	
	if(add!=0)context.state=ADDRESS;
	else context.state=DEF;
	usb_send_status_to_host();
}

static void usb_get_descriptor(usb_dev_reqest_t* req)
{	//most of the things here are hardcoded..
	uint8_t transfer_size=0;
	uint8_t type;
	uint8_t index;
	type=(uint8_t)(req->wValue >>8);
	index=(uint8_t)req->wValue;
	switch(type)
	{
		case DEVICE:
		transfer_size=usb_read_descriptor((uint8_t*)&device_descriptor,0,sizeof(device_descriptor));
		break;
		case CONFIGURATION:
		transfer_size=usb_read_descriptor((uint8_t*)&configuration_descriptor,0,sizeof(configuration_descriptor));
		transfer_size=usb_read_descriptor((uint8_t*)&interface_0_setting_0_descriptor,transfer_size,sizeof(usb_interf_descriptor_t));
		transfer_size=usb_read_descriptor((uint8_t*)&ep_1_interface_0_setting_0_descriptor,transfer_size,sizeof(usb_endpoint_descriptor_t));
		transfer_size=usb_read_descriptor((uint8_t*)&ep_4_interface_0_setting_0_descriptor,transfer_size,sizeof(usb_endpoint_descriptor_t));
		transfer_size=usb_read_descriptor((uint8_t*)&ep_6_interface_0_setting_0_descriptor,transfer_size,sizeof(usb_endpoint_descriptor_t));
		break;
		case STRING: //for the default lang_id
		if(index==0)transfer_size=usb_read_descriptor((uint8_t*)&string_descriptor_0,0,string_descriptor_0.bLength);
		else if(index==1)transfer_size=usb_read_descriptor((uint8_t*)&str_desc_1,0,str_desc_1.bLength);
		else if(index==2)transfer_size=usb_read_descriptor((uint8_t*)&str_desc_2,0,str_desc_2.bLength);
		else {stall_on(0);return;}
		break;
		case INTERFACE:
		if(index!=0){stall_on(0);return;}
		transfer_size=usb_read_descriptor((uint8_t*)&interface_0_setting_0_descriptor,0,sizeof(usb_interf_descriptor_t));
		break;
		case ENDPOINT:
		if(index==1)transfer_size=usb_read_descriptor((uint8_t*)&ep_1_interface_0_setting_0_descriptor,0,sizeof(usb_endpoint_descriptor_t));
		else if(index==4)transfer_size=usb_read_descriptor((uint8_t*)&ep_4_interface_0_setting_0_descriptor,0,sizeof(usb_endpoint_descriptor_t));
		else if(index==6)transfer_size=usb_read_descriptor((uint8_t*)&ep_6_interface_0_setting_0_descriptor,0,sizeof(usb_endpoint_descriptor_t));
		else {stall_on(0);return;}
		break;
		default:
		stall_on(0);
		return;
		break;
		
	}
	
	
	context.dir=IN;
	if(transfer_size>req->wLength)transfer_size=(uint8_t)req->wLength;
	context.transfer_size=transfer_size;
	context.bytes_left=context.transfer_size;
	usb_process_in_setup();
}

static void usb_get_configuration()
{	
	UENUM=0;
	if(context.state==DEF){stall_on(0);return;}
	else if(context.state==ADDRESS)UEDATX=0;
	else if (context.state==CONFIGURED)UEDATX=1;
	//VALID IN TRANSACTION END:
	context.dir=IN;
	context.transfer_size=0;
	context.bytes_left=0;
	ack_in(0);
	
}

static void usb_get_interface(usb_dev_reqest_t* req)
{
	UENUM=0;
	if(context.state==DEF){stall_on(0);return;}
	else if(context.state==ADDRESS){stall_on(0);return;}
	else if (context.state==CONFIGURED)
	{
		if(req->wIndex.word!=0){stall_on(0);return;}
		UEDATX=0;//return default alternate setting
	}
	context.dir=IN;
	context.transfer_size=0;
	context.bytes_left=0;
	ack_in(0);
	
}

static void usb_set_configuration(usb_dev_reqest_t* req)
{	
		if(context.state==DEF){stall_on(0);return;}
		else if(context.state==ADDRESS){
			if(req->wValue==1){context.state=CONFIGURED;
			configure_ep(1);
			clear_toggle(1);
			configure_ep(4);
			clear_toggle(4);
			configure_ep(6);
			clear_toggle(6);
			}else if(req->wValue==0);
			else{stall_on(0);return;}
		}
		else if (context.state==CONFIGURED)
		{
			if(req->wValue==1);
			else if(req->wValue==0)context.state=ADDRESS;
			else{stall_on(0);return;}
		}
	//VALID OUT TRANSACTION END:
	context.dir=OUT;
	usb_send_status_to_host();
}

static void usb_set_interface(usb_dev_reqest_t* req)
{	//wValue=alternate setting, wIndex=interface index
	
	//This device has only one setting and one interface, so
	//it always returns stall even in the configured state
	
	if(context.state==DEF){stall_on(0);return;}
	else if(context.state==ADDRESS){stall_on(0);return;}
	else if(context.state==CONFIGURED){
		stall_on(0);return;
		//normally we would select alternate setting here
		//and reset data toggles on new enpoints with clear_toggle(x)
		}
	
	context.dir=OUT;
	usb_send_status_to_host();
}
//OSRFX2 USB VENDOR COMMANDS:
static void	usb_read_7_segment(usb_dev_reqest_t* req)
{
	UEDATX=osrfx2.sev_segment;
	context.dir=IN;
	context.transfer_size=0;
	context.bytes_left=0;
	ack_in(0);	
}

static void	usb_read_switches(usb_dev_reqest_t* req)
{
		UEDATX=osrfx2.switches;
		context.dir=IN;
		context.transfer_size=0;
		context.bytes_left=0;
		ack_in(0);
}

static void	usb_read_bargraph(usb_dev_reqest_t* req)
{
		UEDATX=osrfx2.bargraph;
		context.dir=IN;
		context.transfer_size=0;
		context.bytes_left=0;
		ack_in(0);
}

static void	usb_set_bargraph(usb_dev_reqest_t* req)
{
	osrfx2.bargraph=UEDATX;
	context.dir=OUT;
	usb_send_status_to_host();
}

static void	usb_is_high_speed(usb_dev_reqest_t* req)
{
		UEDATX=osrfx2.speed;//should be always 0 (usb FS device only)
		context.dir=IN;
		context.transfer_size=0;
		context.bytes_left=0;
		ack_in(0);
}

static void	usb_set_7_segment(usb_dev_reqest_t* req)
{
	osrfx2.sev_segment=UEDATX;
	context.dir=OUT;
	usb_send_status_to_host();
}

static void usb_osrfx2_read_switches()
{
	ep1_context.buff_ptr[0]=osrfx2.switches;
	ep1_context.bytes_left=1;
	ep1_context.transfer_size=1;
	osrfx2.switches++;//change switches state in SW (we have no HW switches)
}

static void inline usb_send_status_to_host()
{
	context.transfer_size=0;
	context.bytes_left=0;
	ack_in(0);
}

static void usb_process_in_setup()
{	
	uint8_t i;
	UENUM=0;
  //status stage
  if(context.bytes_left==0 && UEBCHX==0 && UEBCLX==0 && context.dir==OUT){context.dir=NONE;return;}
 if(context.bytes_left==0 && UEBCHX==0 && UEBCLX==0 && context.dir==ADDR){context.dir=NONE;adden();return;}
 //else queue more data to send
 for(i=0;i<context.ep_data_len&&i<context.bytes_left;i++)UEDATX=context.buff_ptr[context.transfer_size-context.bytes_left+i];
 context.bytes_left-=i;
 //this function never queues status msg, because the host sends that msg
 ack_in(0);
}

static void usb_process_out_setup()
{
  uint8_t i;
  UENUM=0;
  //status stage if data was "IN"
    if( UEBCHX==0 && UEBCLX==0 && context.dir==IN){context.dir=NONE;ack_out(0);context.transfer_size=0;context.bytes_left=0;killbk(0);return;}
   for(i=0;i<context.ep_data_len&&i<context.bytes_left;i++)context.buff_ptr[context.transfer_size-context.bytes_left+i]=UEDATX;
   context.bytes_left-=i;
  
  //set status stage if data was "OUT"
  if(context.bytes_left==0 && context.dir==OUT){usb_send_status_to_host();};//data toggle should be 1 !!!
  //TODO   if(context.bytes_left==0) all bytes are read here, we can call some function if further processing is needed
  
  ack_out(0);
}



static void usb_process_setup()
{			uint8_t i;
			usb_dev_reqest_t *req;
			context.bytes_left=0;
			context.transfer_size=0;
			context.dir=NONE;
			UENUM=0;
			for(i=0;i<context.ep_data_len;i++)context.ctrl_tr[i]=UEDATX;
			req=(usb_dev_reqest_t*)context.ctrl_tr;
			ack_stp(0);//cannot ack before buffer is read
			killbk(0);
			switch(req->bRequest)
			{
				case GET_STATUS:
				 usb_get_status(req);
				break;
				case CLEAR_FEATURE:
				usb_clear_feature(req);
				break;
				case SET_FEATURE:
				usb_set_feature(req);
				break;
				case SET_ADDRESS:
				usb_set_address(req);
				break;
				case GET_DESCRIPTOR:
				usb_get_descriptor(req);
				break;
				case GET_CONFIGURATION:
				usb_get_configuration();
				break;
				case SET_CONFIGURATION:
				usb_set_configuration(req);
				break;
				case GET_INTERFACE:
				usb_get_interface(req);
				break;
				case SET_INTERFACE:
				usb_set_interface(req);
				break;
				case READ_7_SEGMENT:
				usb_read_7_segment(req);
				break;
				case READ_SWITCHES:
				usb_read_switches(req);
				break;
				case READ_BARGRAPH:
				usb_read_bargraph(req);
				break;
				case SET_BARGRAPH:
				usb_set_bargraph(req);
				break;
				case IS_HIGH_SPEED:
				usb_is_high_speed(req);
				break;
				case SET_7_SEGMENT:
				usb_set_7_segment(req);
				break;
				default:
				stall_on(0);
				break;
			}
			
}

static void usb_ep1_process_in()
{	
	uint8_t i;
	UENUM=1;
	//automatically read data from osrfx2.switches
	usb_osrfx2_read_switches();
	 for(i=0;i<ep1_context.ep_data_len&&i<ep1_context.bytes_left;i++)UEDATX=ep1_context.buff_ptr[ep1_context.transfer_size-ep1_context.bytes_left+i];
	 ep1_context.bytes_left-=i;

	 ack_in(1);
	 ack_fifo(1);
}

static void usb_ep4_process_out()
{		
		uint8_t i;
		UENUM=4;
	   for(i=0;i<ep4_context.ep_data_len&&i<ep4_context.bytes_left;i++)ep4_context.buff_ptr[ep4_context.transfer_size-ep4_context.bytes_left+i]=UEDATX;
	   ep4_context.bytes_left-=i;
	   ack_out(4);
	   ack_fifo(4);
	   if(ep4_context.bytes_left==0&&ep4_context.callback!=0)
	   {
		   ep4_context.callback();
		   ep4_context.callback=0;
		   ep4_context.transfer_size=0;
		   ep4_context.buff_ptr=0;
	   }
	   
}

static void usb_ep6_process_in()
{	
	uint8_t i;
	UENUM=6;
	for(i=0;i<ep6_context.ep_data_len&&i<ep6_context.bytes_left;i++)
	{
		UEDATX=ep6_context.buff_ptr[ep6_context.transfer_size-ep6_context.bytes_left+i];
		}
	ep6_context.bytes_left-=i;

		   if(ep6_context.bytes_left==0&&ep6_context.callback!=0)
		   {
			   ep6_context.callback();
			   ep6_context.callback=0;
			   ep6_context.transfer_size=0;
			   ep6_context.buff_ptr=0;
		   }
					ack_in(6);
					ack_fifo(6);
				  
}

uint8_t usb_ep6_write(uint8_t *ptr,uint8_t len,void (*callback)())
{	if(context.state==CONFIGURED){
	cli();
	ep6_context.buff_ptr=ptr;
	ep6_context.transfer_size=len;
	ep6_context.bytes_left=len;
	ep6_context.callback=callback;
	usb_ep6_process_in();
	sei();
	return 1;
	}
	else return 0;
}

uint8_t usb_ep4_read(uint8_t *ptr,uint8_t len,void (*callback)())
{	
	if(context.state==CONFIGURED){
		cli();
	ep4_context.buff_ptr=ptr;
	ep4_context.transfer_size=len;
	ep4_context.bytes_left=len;
	ep4_context.callback=callback;
	sei();
	return 1;
	} else return 0;

}

 ISR(USB_COM_vect){	
		if(UEINT&0x01){//ep0 raised interrupt
			UENUM=0;
			if(UEINTX&(1<<RXSTPI))usb_process_setup();
			if(UEINTX&(1<<TXINI))usb_process_in_setup();
			if(UEINTX&(1<<RXOUTI))usb_process_out_setup();

		}
		
		//TODO here we should also check if context.state==CONFIGURED
		if(UEINT&0x02)
		{		
			UENUM=1;
				if(UEINTX&(1<<TXINI))
				{	
					//if(ep1_context.bytes_left!=0)usb_ep1_process_in(); //send NAK if there is nothing to send
					//in this situation interrupt automatically reads variable, so above is irrelevant
					usb_ep1_process_in();
					
				}
				else{stall_on(2);}
			
			}
		
		if(UEINT&0x10)
		{	UENUM=4;
			if(UEINTX&(1<<RXOUTI))
			{
				if(ep4_context.bytes_left!=0)usb_ep4_process_out();
				//send NAK if there is nothing to rcv
			}
			else{stall_on(4);}
		}
		
		if(UEINT&0x40)
		{	UENUM=6;
			if(UEINTX&(1<<TXINI))
			{
				if(ep6_context.bytes_left!=0)usb_ep6_process_in();
				//send NAK if there is nothing to send
			}
			else{stall_on(6);}
			
			}

		
UENUM=0;
		//TODO add interrupt servicing functions to all
		//other endpoints
		//if unused endpoint was addressed return stall
	
}

ISR(USB_GEN_vect){
	if(UDINT&(1<<EORSTI))
	{
		UDINT&=~(1<<EORSTI);
		configure_ep(0);
		context.state=DEF;
		context.remote_wakeup=0;
		
	}
	if(UDINT&(1<<SOFI))UDINT&=~(1<<SOFI);
	if(UDINT&(1<<SUSPI))UDINT&=~(1<<SUSPI);
	if(UDINT&(1<<WAKEUPI))UDINT&=~(1<<WAKEUPI);
	
}
