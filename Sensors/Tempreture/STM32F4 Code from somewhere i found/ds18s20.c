#include  <stdio.h>


/*
 *  stm32f4_discovery.h is located in Utilities/STM32F4-Discovery
 *  and defines the GPIO Pins where the leds are connected.
 *  Including this header also includes stm32f4xx.h and
 *  stm32f4xx_conf.h, which includes stm32f4xx_gpio.h
 */
#include "stm32f4_discovery.h"


#include  "uart.h"
#include  "term_io.h"

/*
 *  Include all needed standard peripheral driver headers.
 */
#include "core_cm4.h"
#include "misc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

/*
 *  Specify the port and pin used for 1-wire comms
 */
#define ONEWIRE_PIN_NUM					1
#define ONEWIRE_PIN_MASK                (1<<ONEWIRE_PIN_NUM)
#define ONEWIRE_PORT					GPIOC
#define ONEWIRE_CLK						RCC_AHB1Periph_GPIOC  


/*
 *  The following macros collapse direct accesses of the GPIO registers into
 *  single commands.  Refer to stm32f4xx_gpio.c and the STM32F4xx Reference
 *  Manual (GPIO chapter) for details.
 */
#define  ONEWIRE_INPUT_READ				ONEWIRE_PORT->IDR&ONEWIRE_PIN_MASK
#define  ONEWIRE_OUTPUT_HIGH			ONEWIRE_PORT->BSRRL=ONEWIRE_PIN_MASK
#define  ONEWIRE_OUTPUT_LOW				ONEWIRE_PORT->BSRRH=ONEWIRE_PIN_MASK
#define  ONEWIRE_CONFIG_OUTPUT			ONEWIRE_PORT->MODER|=(GPIO_Mode_OUT<<(ONEWIRE_PIN_NUM*2))
#define  ONEWIRE_CONFIG_INPUT			ONEWIRE_PORT->MODER&=~(GPIO_MODER_MODER0<<(ONEWIRE_PIN_NUM*2))


#define  READ_ROM			0x33
#define  SKIP_ROM			0xcc
#define  READ_SCRATCHPAD	0xbe
#define  CONVERT_TEMP		0x44


/*
 *  Local functions
 */
static void					PingOneWireNetwork(void);
static void					OneWire_Init(void);
static void					delay_usecs(uint32_t  usecs);
static void					SendInitialization(void);
static void					SendByte(uint8_t  val);
static uint8_t				ReadByte(void);
static void					ReportScratchpad(void);
static void					StartConversion(void);
static void					ReportTemperature(void);
static void					ReportROM(void);



uint8_t						pad[9];
uint8_t						rom[8];


int  main(void)
{
	SystemInit();

	UARTInit(2, 115200);
	OneWire_Init();

	xputs("\n\rDallas DS18S20 demo\n\r");


	PingOneWireNetwork();
	ReportROM();
	ReportScratchpad();
	StartConversion();
	delay_usecs(750000);
	ReportTemperature();
	ReportScratchpad();

	while (1)   ;
	return  0;
}



static void  PingOneWireNetwork(void)
{
	uint8_t				response;

	xputs("\n\rSending initialization pulses...");

	response = 0;

	ONEWIRE_CONFIG_INPUT;
	delay_usecs(100);

	while (1)
	{
		SendInitialization();
		response = ONEWIRE_INPUT_READ;		// device pulls line low in response
		delay_usecs(420);

		if (response == 0)  break;
	}
	xputs("response detected.");
}



static void  StartConversion(void)
{
	xputs("\n\rSending command to start conversion.");
	SendInitialization();
	delay_usecs(100);
	SendByte(SKIP_ROM);
	SendByte(CONVERT_TEMP);
}


static void  ReportTemperature(void)
{
	uint32_t			val;
	uint32_t			t;
	uint32_t			frac;
	uint8_t				n;

	SendInitialization();
	delay_usecs(100);
	SendByte(SKIP_ROM);
	SendByte(READ_SCRATCHPAD);
	for (n=0; n<9; n++)
	{
		pad[n] = ReadByte();
	}
	val = (pad[1] * 256 + pad[0]);			// temp in 0.5 degs C
	t = val;
	val = val >> 1;							// temp in degs C
	frac = 0;
	if ((val << 1) != t)  frac = 5;			// if the roll lost a bit, allow for 0.5 deg C
	xprintf("\n\rTemperature is: %d.%d degrees C", val, frac);
}



static void  ReportROM(void)
{
	uint8_t					n;

	SendInitialization();
	delay_usecs(100);
	SendByte(READ_ROM);
	for (n=0; n<8; n++)
	{
		rom[n] = ReadByte();
	}
	xprintf("\n\rROM:  7  6  5  4  3  2  1  0");
	xprintf("\n\r     %02x %02x %02x %02x %02x %02x %02x %02x",
		rom[7], rom[6], rom[5], rom[4], rom[3], rom[2], rom[1], rom[0]);
}



static void  ReportScratchpad(void)
{
	uint8_t					n;

	SendInitialization();
	delay_usecs(100);
	SendByte(SKIP_ROM);
	SendByte(READ_SCRATCHPAD);
	for (n=0; n<9; n++)
	{
		pad[n] = ReadByte();
	}
	xprintf("\n\rScratchpad contains: %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		pad[0], pad[1], pad[2], pad[3], pad[4], pad[5], pad[6], pad[7], pad[8]);
}


static void  SendInitialization(void)
{
	ONEWIRE_OUTPUT_HIGH;
	ONEWIRE_CONFIG_OUTPUT;
	delay_usecs(500);

	ONEWIRE_OUTPUT_LOW;
	delay_usecs(500);

	ONEWIRE_OUTPUT_HIGH;
	ONEWIRE_CONFIG_INPUT;
	delay_usecs(50);
}


static void  SendByte(uint8_t  val)
{
	uint8_t				n;

	for (n=0; n<8; n++)
	{
		ONEWIRE_OUTPUT_LOW;
		ONEWIRE_CONFIG_OUTPUT;
		delay_usecs(5);
		if (val & 1)  ONEWIRE_OUTPUT_HIGH;
		delay_usecs(95);
		ONEWIRE_OUTPUT_HIGH;
		delay_usecs(5);
		val = val >> 1;
	}
}



static  uint8_t  ReadByte(void)
{
	uint8_t				n;
	uint8_t				val;

	val = 0;
	for (n=0; n<8; n++)
	{
		val = val >> 1;
		ONEWIRE_OUTPUT_LOW;
		ONEWIRE_CONFIG_OUTPUT;
		delay_usecs(15);
		ONEWIRE_OUTPUT_HIGH;
		ONEWIRE_CONFIG_INPUT;
		delay_usecs(10);
		if (ONEWIRE_INPUT_READ)  val = val | 0x80;
		delay_usecs(35);
	}
	return  val;
}




/*
 *  OneWire_Init      hardware-specific configuration of 1-wire I/O
 */
static void  OneWire_Init(void)
{
	GPIO_InitTypeDef			GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(ONEWIRE_CLK, ENABLE);		// route the clocks

    GPIO_InitStructure.GPIO_Pin = ONEWIRE_PIN_MASK;				// select the pin to modify
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;				// set the mode to output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			// set the I/O speed to 100 MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;				// set the output type to open-drain
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			// set the pull-up to none
    GPIO_Init(ONEWIRE_PORT, &GPIO_InitStructure);				// do the init
}


static void  delay_usecs(uint32_t  usecs)
{
	uint32_t			n;

	n = (144 * usecs) / 10;

	while (n)
	{
		n--;
	}
}


