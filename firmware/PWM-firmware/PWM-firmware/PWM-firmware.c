/*
 * PWM_firmware.c
 *
 * Created: 4/13/2014 10:03:55 PM
 *  Author: Pierce
 */ 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "RGB-Matrix-protocol.h"

#define BUFLEN	200
#define OUTLEN	192
#define PWM_ALL	OCR1A	 
#define PWM_RED	OCR1B
#define PWM_GRN	OCR2B
#define PWM_BLU	OCR2A

uint8_t getAddress (void);
void init (uint8_t * in, uint8_t inlen, uint8_t * out, uint8_t outlen, uint8_t address);
void clearBuf (uint8_t * buf, uint8_t len);
void copyGRBBuf (uint8_t * in, uint8_t * out);
void copy3BitBuf (uint8_t * in, uint8_t * out);

uint8_t inbuf[BUFLEN];
uint8_t output[OUTLEN];
uint8_t inbufPtr;
uint8_t cmd = 0;
void inline doTick (void);

uint16_t pwmCtr;
uint8_t stopFlag = 1;

ISR(TWI_SLAVE_vect) {
	//volatile uint8_t dummy;
	
	if (TWSSRA & _BV(TWDIF)) {						// if we have a data value, put it in the buffer
		inbuf[inbufPtr] = TWSD;						// read in the data
		TWSCRB = _BV(TWCMD0) | _BV(TWCMD1);			// ACK the byte
		inbufPtr++;									// increment the buffer pointer
	} 
	if (TWSSRA & _BV(TWASIF)) {
		if (TWSSRA & _BV(TWAS)) {					// if we have an address, reset the input buffer
			TWSCRB = _BV(TWCMD0) | _BV(TWCMD1);		// ACK the byte; this is first since all our house keeping takes less 
													// time than getting our next byte, so we should do it first to speed the transaction
			TWSSRA |= _BV(TWASIF);					// clear the address match flag
			if (stopFlag) {							// if the last interrupt was caused by a stop condition, clear the buffer
				inbufPtr = 0;
				clearBuf(inbuf, (uint8_t)BUFLEN);
				stopFlag = 0;
			}
		} else {									// if we have a stop condition, set the command to the first byte
			TWSCRB = _BV(TWCMD0) | _BV(TWCMD1);		// ACK the byte
			TWSSRA |= _BV(TWASIF);					// clear the address match flag
			if (inbufPtr) cmd = inbuf[0];
			stopFlag++;
		}
	} 
	if ((TWSSRA & _BV(TWBE)) || 
		(TWSSRA & _BV(TWC))) {						// if we have an error, reset the input buffer and remove the error
		inbufPtr = 0;
		clearBuf(inbuf, (uint8_t)BUFLEN);
		stopFlag++;
		TWSSRA |= _BV(TWBE);						// reset the bus error
		TWSSRA |= _BV(TWC);							// reset the bus collision
		TWSCRB = _BV(TWCMD0) | _BV(TWCMD1);			// ACK the byte
	}
}


int main(void) {
		
	init(inbuf, (uint8_t)BUFLEN, output, (uint8_t)OUTLEN, getAddress());
	
    while(1) {
		doTick();
        switch (cmd) {
			case RGB_CMD_ALL_DIM:
				PWM_ALL = inbuf[1];
			break;
			case RGB_CMD_BLU_DIM:
				PWM_BLU = inbuf[1];
			break;
			case RGB_CMD_GRN_DIM:
				PWM_GRN = inbuf[1];
			break;
			case RGB_CMD_RED_DIM:
				PWM_RED = inbuf[1];
			break;
			case RGB_CMD_24_GRB_DATA:
				copyGRBBuf(inbuf, output);
				cmd = 0;
			break;
			case RGB_CMD_3_RGB_DATA:
				copy3BitBuf(inbuf, output);
				cmd = 0;
			break;
			default:	
			break;
		}
    }
}

uint8_t getAddress (void) {

	//uint8_t pa = 0;
	//uint8_t pb = 0;
	uint8_t result = 0x81;	// set the top bit of the address in order to avoid stomping on the general call address
							// set the bottom bit so that it responds to the general call address 

	// set all lines to inputs
	DDRA = 0;
	DDRB = 0;
	
	// turn on the pull-ups for all address lines in 
	// order to force a default address
	//PUEA = 0xa5;
	//PUEB = 0x05;
	
	// read the input ports
	//pa = PINA;
	//pb = PINB;
	
	// map address pins into an address
	if (PINA & _BV(5)) result += 0x02;	// address bit 0
	if (PINA & _BV(7)) result += 0x04;	// address bit 1
	if (PINA & _BV(0)) result += 0x08;	// address bit 2
	if (PINB & _BV(2)) result += 0x10;	// address bit 3
	if (PINA & _BV(2)) result += 0x20;	// address bit 4
	if (PINA & _BV(1)) result += 0x40;	// address bit 5
	
	// turn off the pull-ups
	//PUEA = 0;
	//PUEB = 0;
	
	return result;
}

void init (uint8_t * in, uint8_t inlen, uint8_t * out, uint8_t outlen, uint8_t address) {
	
	DDRA = 0xaf;	// set everything except the TWI pins to output 
	DDRB = 0x04;	// set everything to outputs except clock lines & reset
	
	// set up PWM outputs
	TCCR1A = _BV(COM1A0) | _BV(COM1A1) | _BV(COM1B0) | _BV(COM1B1) | _BV(WGM10);
	TCCR2A = _BV(COM2A0) | _BV(COM2A1) | _BV(COM2B0) | _BV(COM2B1) | _BV(WGM20);
	TCCR1B = _BV(WGM12)	| _BV(CS10);
	TCCR2B = _BV(WGM22) | _BV(CS20);
	TOCPMSA0 = _BV(TOCC1S0);	
	TOCPMSA1 = _BV(TOCC4S0) | _BV(TOCC6S1) | _BV(TOCC7S1);
	TOCPMCOE = _BV(TOCC1OE) | _BV(TOCC4OE) | _BV(TOCC6OE) | _BV(TOCC7OE);
	PWM_ALL = 0xff;
	PWM_BLU = 0xff;
	PWM_GRN = 0xff;
	PWM_RED = 0xff;
	
	// set up the TWI interface and clear the buffers
	TWSA = address;
	TWSCRA = _BV(TWDIE) | _BV(TWASIE) | _BV(TWEN) | _BV(TWSIE); // | _BV(TWSME);
	//TWSCRB = _BV(TWCMD0) | _BV(TWCMD1);
	clearBuf(in, inlen);
	clearBuf(out, outlen);
	
	// set up SPI output
	SPCR = _BV(SPE) | _BV(MSTR);	// turn on the SPI bus in master mode
	SPSR = _BV(SPI2X);				// turn it up to max speed (clk/2)
	REMAP = _BV(SPIMAP);			// remap the SPI pins to alternates
	
	DDRA = 0xaf;	// set everything except the TWI pins to output (makes sure PA0 is an output)
	PUEA |= _BV(4) | _BV(6);
	
	// turn on interrupts and away we go
	sei();
}

void clearBuf (uint8_t * buf, uint8_t len) {
	int i;
	for (i = 0; i < len; i++) buf[i] = 0;
}

void copyGRBBuf(uint8_t * in, uint8_t * out)
{
	uint8_t row, col;
	
	for (row = 0; row < 8; row++) {
		for (col = 0; col < 8; col++) {
			out[(15-col) + (row * 24)]		= in[((col*3) + 1) + (row * 24)] >> 2;	// copy the green (order is flipped)
			out[col + (row * 24)]			= in[((col*3) + 3) + (row * 24)] >> 2;	// copy the blue
			out[(col + 16) + (row * 24)]	= in[((col*3) + 2) + (row * 24)] >> 2;	// copy the red
		}
	}
}

// Copy the R...G...B... bit string from a buffer encoded for the original buffer
// to the GRBGRBGRB... byte string required for PWM generation in this code
void copy3BitBuf(uint8_t * in, uint8_t * out)
{
	uint8_t row, col;
	
	for (row = 0; row < 8; row++) {
		for (col = 0; col < 8; col++) {
			(in[row + 1] & _BV(col))	? out[((col + 16) + (row * 24))] = 0x3f	: (out[((col + 16) + (row * 24))] = 0);		// grab the red (first eight bytes of input)
			(in[row + 9] & _BV(col))	? out[(col + 8) + (row * 24)] = 0x3f	: (out[(col + 8) + (row * 24)] = 0);		// grab the green (next eight bytes of input)
			(in[row + 17] & _BV(col))	? out[(col) + (row * 24)] = 0x3f		: (out[(col) + (row * 24)] = 0);			// grab the green (next eight bytes of input)
		}
	}
}

void inline doTick(void) {
	uint8_t select = 0;
	uint8_t rowSelect = 0;
	register uint8_t out;
	
	for (rowSelect = 0; rowSelect < 8; rowSelect++) {
		select = rowSelect * 24;
		for (pwmCtr = 0; pwmCtr < 64; pwmCtr++) {
			// Since we're overloading what would otherwise be the MISO line to be the latch line, we need to disable & re-enable SPI every cycle
			SPCR = 0;
			PORTA &= ~_BV(0);	
			PORTA |= _BV(0);
			SPCR = _BV(SPE) | _BV(MSTR);	// turn on the SPI bus in master mode
			
			//PORTA |= _BV(0);					// drive the latch line high
			//SPDR = ~_BV(rowSelect);
			//PORTA &= ~_BV(0);					// clear the latch line
			// write out the blue LEDs
			SPDR = ~_BV(rowSelect);
			out = 0xff;
			(output[select + 0] <= pwmCtr)	? (out |= _BV(0)) : (out &= ~_BV(0));
			(output[select + 1] <= pwmCtr)	? (out |= _BV(1)) : (out &= ~_BV(1));
			(output[select + 2] <= pwmCtr)	? (out |= _BV(2)) : (out &= ~_BV(2));
			(output[select + 3] <= pwmCtr)	? (out |= _BV(3)) : (out &= ~_BV(3));
			(output[select + 4] <= pwmCtr)	? (out |= _BV(4)) : (out &= ~_BV(4));
			(output[select + 5] <= pwmCtr)	? (out |= _BV(5)) : (out &= ~_BV(5));
			(output[select + 6] <= pwmCtr)	? (out |= _BV(6)) : (out &= ~_BV(6));
			(output[select + 7] <= pwmCtr)	? (out |= _BV(7)) : (out &= ~_BV(7));
			
			SPDR = out;
			// write out the green LEDs
			out = 0xff;
			(output[select + 8] <= pwmCtr)	? (out |= _BV(0)) : (out &= ~_BV(0));
			(output[select + 9] <= pwmCtr)	? (out |= _BV(1)) : (out &= ~_BV(1));
			(output[select + 10] <= pwmCtr)	? (out |= _BV(2)) : (out &= ~_BV(2));
			(output[select + 11] <= pwmCtr)	? (out |= _BV(3)) : (out &= ~_BV(3));
			(output[select + 12] <= pwmCtr)	? (out |= _BV(4)) : (out &= ~_BV(4));
			(output[select + 13] <= pwmCtr)	? (out |= _BV(5)) : (out &= ~_BV(5));
			(output[select + 14] <= pwmCtr)	? (out |= _BV(6)) : (out &= ~_BV(6));
			(output[select + 15] <= pwmCtr)	? (out |= _BV(7)) : (out &= ~_BV(7));
			SPDR = out;
			// write out the red LEDs
			out = 0xff;
			(output[select + 16] <= pwmCtr)	? (out |= _BV(0)) : (out &= ~_BV(0));
			(output[select + 17] <= pwmCtr)	? (out |= _BV(1)) : (out &= ~_BV(1));
			(output[select + 18] <= pwmCtr)	? (out |= _BV(2)) : (out &= ~_BV(2));
			(output[select + 19] <= pwmCtr)	? (out |= _BV(3)) : (out &= ~_BV(3));
			(output[select + 20] <= pwmCtr)	? (out |= _BV(4)) : (out &= ~_BV(4));
			(output[select + 21] <= pwmCtr)	? (out |= _BV(5)) : (out &= ~_BV(5));
			(output[select + 22] <= pwmCtr)	? (out |= _BV(6)) : (out &= ~_BV(6));
			(output[select + 23] <= pwmCtr)	? (out |= _BV(7)) : (out &= ~_BV(7));
			SPDR = out;
			while (!(SPSR & _BV(SPIF)));	// wait for the last SPI transmission to finish
			//PORTB |= _BV(1);			// drive the latch line high
		}
	}
}