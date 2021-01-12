#include <avr/pgmspace.h>
#define _AP __attribute__((packed))
//#pragma warning( disable : ;)
/*#define usb_string_descriptor(name,s)const struct { \
	uint8_t bLength; \
	uint8_t bDescriptorType; \
	uint16_t bString[sizeof(s)]; \
	}_AP name  PROGMEM = {.bLength=sizeof(name),.bDescriptorType=STRING};*/

typedef struct
{
	union{
		struct
		{	
			uint8_t Recipient:5;
			uint8_t Type:2;
			uint8_t Dir :1;
			
			
		}bits;
		uint8_t byte;
	}bmRequestType;
	
	uint8_t bRequest;
	uint16_t wValue;
	
	union{
		struct{
			uint16_t Endpoint:4;	
			uint16_t Reservedl:3;//0
			uint16_t Dir:1;
			uint16_t Reservedh:8;//0
			
		}bits;
		
		struct{
			uint8_t low;
			uint8_t high;
			
		}bytes;
		struct{
			uint8_t Interface;
			uint8_t Reserved;
		}byte;
		uint16_t word;
	} wIndex;
	uint16_t wLength;
} _AP usb_dev_reqest_t;

//usb_dev_req_t usb_dev_req;

//transfer direction
#define REQ_DIR_OUT 0
#define REQ_DIR_IN 1
//standard descriptor type
#define REQ_TYPE_ST 0
//request recipients
#define REQ_RECIPIENT_DE 0
#define REQ_RECIPIENT_IF 1
#define REQ_RECIPIENT_EP 2
//standard request codes
#define GET_STATUS 0
#define CLEAR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADDRESS 5
#define GET_DESCRIPTOR 6
#define SET_DESCRIPTOR 7
#define GET_CONFIGURATION 8
#define SET_CONFIGURATION 9
#define GET_INTERFACE 10
#define SET_INTERFACE 11
#define SYNCH_FRAME 12
//descriptor types
#define DEVICE 1
#define CONFIGURATION 2
#define STRING 3
#define INTERFACE 4
#define ENDPOINT 5
#define DEVICE_QUALIFIER 6
#define OTHER_SPEED_CONFIGURATION 7
#define INTERFACE_POWER 8
//standard feature selectors
#define DEVICE_REMOTE_WAKEUP 1 //recipient=DEVICE
#define ENDPOINT_HALT 0 //recipient=ENDPOINT
#define TEST_MODE 2 //recipient=DEVICE
//GET_STATUS misc:
#define REMOTE_WAKEUP 2 //device
#define SELF_POWERED 1 //device
//interface 0
#define HALT 1

typedef struct{
	uint8_t bLength;
	uint8_t bDescirptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
}_AP usb_dev_descriptor_t;


typedef struct{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	
	union{
		struct
		{		
			uint8_t Reserved2:5;
			uint8_t RemoteWakup:1;
			uint8_t SelfPowered:1;
			uint8_t Reserved:1; //set to 1
		}bits;
		uint8_t byte;
	} bmAttributes;
	uint8_t bMaxPower;
}_AP usb_conf_descriptor_t;

typedef struct{
	uint8_t bLength;
	uint8_t bDecriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
	
}_AP usb_interf_descriptor_t;

typedef struct{
	uint8_t bLength;
	uint8_t bDescriptorType;
	union{
		struct{
			uint8_t Number:4;
			uint8_t Reserved:3;		
			uint8_t Dir:1;
		}bits;
	}bEndpointAddress;
	
	union{
		struct{
			uint8_t TrasnferType:2;
			uint8_t SyncType:2;
			uint8_t UsageType:2;		
			uint8_t Reserved:2;
			
		}bits;
	}bmAttributes;
	union{
		
		struct{
			uint16_t PacketSize:11;
			uint16_t AdditionalOp:2;
			uint16_t Reserved:3;

		}bits;
		
		
	} wMaxPacketSize;
	uint8_t bInterval;
}_AP usb_endpoint_descriptor_t;

#define TRANSFER_CONTROL 0
#define TRANSFER_ISO	1
#define TRANSFER_BULK	2
#define TRANSFER_INT	3
#define SYNTYPE_NO		0
#define SYNTYPE_ASYNCH	1
#define SYNTYPE_ADAPTIVE 2
#define SYNTYPE_SYNCH 3
#define USAGETYPE_DATA 0
#define USAGETYPE_FEEDBACK 1
#define USAGETYPE_IMPLICIT 2

#define NUM_LANGID 1
typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wLANGID[NUM_LANGID];
}_AP usb_string_descriptor_zero_t;

typedef struct{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[];
}_AP usb_string_descriptor_t;





//DESCRIPTORS ARE DECLARED HERE!!
const usb_dev_descriptor_t  device_descriptor PROGMEM = {
	.bLength=sizeof(device_descriptor),
	.bDescirptorType=DEVICE,
	.bcdUSB=0x0200,
	.bDeviceClass=0xFF,
	.bDeviceSubClass=0x00,
	.bDeviceProtocol=0x00,
	.bMaxPacketSize=0x08,
	.idVendor=0x0547,
	.idProduct=0x1002,
	.bcdDevice=0x0000,
	.iManufacturer=0x01,
	.iProduct=0x02,
	.iSerialNumber=0x00,
.bNumConfigurations=0x01};

usb_conf_descriptor_t const configuration_descriptor PROGMEM ={
	.bLength=sizeof(configuration_descriptor),
	.bDescriptorType=CONFIGURATION,
	.wTotalLength=sizeof(configuration_descriptor)+sizeof(usb_interf_descriptor_t)
	+3*sizeof(usb_endpoint_descriptor_t),
	.bNumInterfaces=0x01,
	.bConfigurationValue=0x01,
	.iConfiguration=0x00,

	.bmAttributes.bits.Reserved=1, //set to 1
	.bmAttributes.bits.SelfPowered=0,
	.bmAttributes.bits.RemoteWakup=0,
	.bmAttributes.bits.Reserved2=0,
	
	.bMaxPower=0x32 //100ma
};
usb_interf_descriptor_t const interface_0_setting_0_descriptor PROGMEM =
{
	.bLength=sizeof(usb_interf_descriptor_t),
	.bDecriptorType=INTERFACE,
	.bInterfaceNumber=0,
	.bAlternateSetting=0,
	.bNumEndpoints=3,
	.bInterfaceClass=0xFF,
	.bInterfaceSubClass=0x00,
	.bInterfaceProtocol=0x00,
	.iInterface=0x00
};
//init


usb_endpoint_descriptor_t const ep_1_interface_0_setting_0_descriptor PROGMEM={
	.bLength=sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType=ENDPOINT,
	.bEndpointAddress.bits.Dir=1, // IN
	.bEndpointAddress.bits.Reserved=0,
	.bEndpointAddress.bits.Number=1,
	.bmAttributes.bits.Reserved=0,
	.bmAttributes.bits.UsageType=0,
	.bmAttributes.bits.SyncType=0,
	.bmAttributes.bits.TrasnferType=3,//interrupt
	.wMaxPacketSize.bits.Reserved=0,
	.wMaxPacketSize.bits.AdditionalOp=0,
	.wMaxPacketSize.bits.PacketSize=8,
	.bInterval=8 //2**(n-1) polling rate
	
};

usb_endpoint_descriptor_t const ep_4_interface_0_setting_0_descriptor PROGMEM={
	.bLength=sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType=ENDPOINT,
	.bEndpointAddress.bits.Dir=0, // OUT
	.bEndpointAddress.bits.Reserved=0,
	.bEndpointAddress.bits.Number=4,
	.bmAttributes.bits.Reserved=0,
	.bmAttributes.bits.UsageType=0,
	.bmAttributes.bits.SyncType=0,
	.bmAttributes.bits.TrasnferType=2,//bulk
	.wMaxPacketSize.bits.Reserved=0,
	.wMaxPacketSize.bits.AdditionalOp=0,
	.wMaxPacketSize.bits.PacketSize=64,
	.bInterval=255 //nak rate
	
};
usb_endpoint_descriptor_t const ep_6_interface_0_setting_0_descriptor PROGMEM={
	.bLength=sizeof(usb_endpoint_descriptor_t),
	.bDescriptorType=ENDPOINT,
	.bEndpointAddress.bits.Dir=1, // IN
	.bEndpointAddress.bits.Reserved=0,
	.bEndpointAddress.bits.Number=6,
	.bmAttributes.bits.Reserved=0,
	.bmAttributes.bits.UsageType=0,
	.bmAttributes.bits.SyncType=0,
	.bmAttributes.bits.TrasnferType=2,//bulk
	.wMaxPacketSize.bits.Reserved=0,
	.wMaxPacketSize.bits.AdditionalOp=0,
	.wMaxPacketSize.bits.PacketSize=64,
	.bInterval=255 //nak rate
	
};

usb_string_descriptor_zero_t const string_descriptor_0 PROGMEM =
{
	.bLength=sizeof(usb_string_descriptor_zero_t),
	.bDescriptorType=STRING,
	.wLANGID[0]=0x0409
};


const struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bString[sizeof("Manufacturer")];
	}_AP str_desc_1  PROGMEM = {.bLength=sizeof(str_desc_1),.bDescriptorType=STRING,.bString={'M','a','n','u','f','a','c','t','u','r','e','r'}};
const struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bString[sizeof("Product")];
	}_AP str_desc_2  PROGMEM = {.bLength=sizeof(str_desc_2),.bDescriptorType=STRING,.bString={'P','r','o','d','u','c','t'}};

