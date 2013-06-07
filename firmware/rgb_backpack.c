// low fuse setting:		0xef
// high fuse setting:		0xdf
// extended fuse setting: 	0xff
// avrdude fuse command line: "avrdude -c usbtiny -p t2313 -Ulfuse:w:0xe4:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m"
// avrdude flash command line: "avrdude  -c usbtiny -p t2313 -Uflash:w:rgb_backpack.hex"
// avrdude whole command line: "avrdude -c usbtiny -p t2313 -Ulfuse:w:0xe4:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m -Uflash:w:rgb_backpack.hex"

#include <avr/io.h>    		// for PORTx, PINx, DDRx, etc.
#include <avr/interrupt.h>  // there will be interrupts
#include <stdint.h>			// for all those tasty integer types
#include <avr/sfr_defs.h>	// some useful stuff for manipulation of registers
#include <util/delay.h>
#include <util/twi.h>		// bitmask for I2C
#include "usiTwiSlave.h"

// Define the address setting jumpers

#define ADDL_PORT	PINB
#define ADDH_PORT	PIND

// Define the lines for driving the backpack chip

#define DRV_PORT 	PORTD
#define DRV_CS		PD6
#define DRV_RD		PD5
#define DRV_WR		PD2
#define DRV_DATA	PD1

// Define commands

#define CMD_DISABLE		0x00
#define CMD_ENABLE		0x01
#define CMD_LEDOFF		0x02
#define CMD_LEDON		0x03
#define CMD_BLINKON		0x08
#define CMD_BLINKOFF	0x09
#define CMD_SLAVE		0x10
#define CMD_MASTER_RC	0x18
#define CMD_MASTER_EXT	0x1c
#define CMD_COM_N8		0x20
#define CMD_COM_N16		0x24
#define CMD_COM_P8		0x28
#define CMD_COM_P16		0x2c

void init (void);
void shiftOut (uint8_t *bytes); 
void shiftCmd (uint8_t cmd);
void shiftByte (uint8_t byte);
void shiftBuffer (void);

int main (void) {

	
	init();
	
	while (1) {
		if(usiTwiDataInReceiveBuffer()) {
			_delay_ms(10.0);
			shiftBuffer();
		}
	}
	
}

void init (void) {
	uint8_t address = 0x00;
	//uint8_t bytes[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	// set up ports
	DDRD = 0xe6;
	DDRB = 0xe9;
	
	// initialize driver
	DRV_PORT |= (_BV(DRV_CS) | _BV(DRV_WR) | _BV(DRV_RD) | _BV(DRV_DATA));
	//shiftCmd(CMD_MASTER_RC);
	shiftCmd(CMD_MASTER_EXT);
	shiftCmd(CMD_COM_P8);
	shiftCmd(CMD_ENABLE);
	shiftCmd(CMD_LEDON);
	
	// start the timer
	TCCR1A 	= _BV(COM1A0);					// Toggle on match, channel A
	TCCR1B 	= _BV(WGM12) | _BV(CS10);		// CTC mode, no prescaling
	OCR1A	= 3;							// 1 MHz		
	
	// read address
	PORTB |= (_BV(PB4)|_BV(PB2)|_BV(PB1));			// set pullups high to enable default address
	PORTD |= (_BV(PD0)|_BV(PD3)|_BV(PD4));			// set pullups high to enable default address
	if ((PINB & _BV(PB4)) != 0) address |= 0x01; 
	if ((PINB & _BV(PB2)) != 0) address |= 0x02;
	if ((PINB & _BV(PB1)) != 0) address |= 0x04;
	if ((PIND & _BV(PD0)) != 0) address |= 0x08; 
	if ((PIND & _BV(PD3)) != 0) address |= 0x10;
	if ((PIND & _BV(PD4)) != 0) address |= 0x20;
	address |= 0x40;	// set the high bit, so we don't squat on the broadcast address (0x00)
	
	// check address
	//bytes[0] = address;
	//bytes[1] = ~address;
	//shiftOut(bytes);
	
	// initialize USI for I2C slave
	usiTwiSlaveInit(address);
	
	// enable interrupts
	sei();
	return;
}

void shiftOut (uint8_t *bytes) {

	uint8_t i, j, val;
	
	DRV_PORT &= ~_BV(DRV_CS);		// enable the serial bus
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a one
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a zero
	DRV_PORT |= _BV(DRV_DATA);
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a one	
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	
	// set the address to 0x00
	for (i = 0; i < 7; i++)  {
		DRV_PORT &= ~_BV(DRV_WR);
		DRV_PORT |= _BV(DRV_WR);
	}
	
	// shift the data
	for (i = 0; i < 24; i++) {
		val = bytes[i];
		for (j = 0; j < 8; j++)  {
			DRV_PORT &= ~_BV(DRV_WR);	// prep for next bit
			if (val & (1 << j)) {
				DRV_PORT |= _BV(DRV_DATA);
			} else {
				DRV_PORT &= ~_BV(DRV_DATA);
			}
			DRV_PORT |= _BV(DRV_WR);	// clock in the next bit
		}
	}
	DRV_PORT |= _BV(DRV_WR);
	DRV_PORT |= _BV(DRV_DATA);
	DRV_PORT |= _BV(DRV_CS);		// end transmission
	return;
}

void shiftCmd (uint8_t cmd) {
	uint8_t i;

	DRV_PORT &= ~_BV(DRV_CS);		// enable the serial bus
	_delay_us(1.0);	
	DRV_PORT &= ~_BV(DRV_WR);
	_delay_us(1.0);
	DRV_PORT |= _BV(DRV_WR);		// clock in a one
	_delay_us(1.0);
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	_delay_us(1.0);
	DRV_PORT |= _BV(DRV_WR);		// clock in a zero
	_delay_us(1.0);
	DRV_PORT &= ~_BV(DRV_WR);
	_delay_us(1.0);
	DRV_PORT |= _BV(DRV_WR);		// clock in a zero

	for (i = 0; i < 8; i++)  {
		DRV_PORT &= ~_BV(DRV_WR);	// prep for next bit
		if (cmd & (1 << (7 - i))) {
			DRV_PORT |= _BV(DRV_DATA);
		} else {
			DRV_PORT &= ~_BV(DRV_DATA);
		}
		_delay_us(1.0);
		DRV_PORT |= _BV(DRV_WR);	// clock in the next bit
		_delay_us(1.0);
	}
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	_delay_us(1.0);
	DRV_PORT |= _BV(DRV_WR);		// shift out a trailing zero
	_delay_us(1.0);
	DRV_PORT |= _BV(DRV_DATA);
	DRV_PORT |= _BV(DRV_CS);		// end transmission
	return;
}
void shiftBuffer (void) {

	uint8_t i, j, val;
	
	DRV_PORT &= ~_BV(DRV_CS);		// enable the serial bus
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a one
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a zero
	DRV_PORT |= _BV(DRV_DATA);
	DRV_PORT &= ~_BV(DRV_WR);
	DRV_PORT |= _BV(DRV_WR);		// clock in a one	
	DRV_PORT &= ~_BV(DRV_DATA);	// set data output to zero
	DRV_PORT &= ~_BV(DRV_WR);
	
	// set the address to 0x00
	for (i = 0; i < 7; i++)  {
		DRV_PORT &= ~_BV(DRV_WR);
		DRV_PORT |= _BV(DRV_WR);
	}
	
	// shift the data
	for (i = 0; i < 24; i++) {
		val = usiTwiReceiveByte();
		for (j = 0; j < 8; j++)  {
			DRV_PORT &= ~_BV(DRV_WR);	// prep for next bit
			if (val & (1 << j)) {
				DRV_PORT |= _BV(DRV_DATA);
			} else {
				DRV_PORT &= ~_BV(DRV_DATA);
			}
			DRV_PORT |= _BV(DRV_WR);	// clock in the next bit
		}
	}
	DRV_PORT |= _BV(DRV_WR);
	DRV_PORT |= _BV(DRV_DATA);
	DRV_PORT |= _BV(DRV_CS);		// end transmission
	return;

}
