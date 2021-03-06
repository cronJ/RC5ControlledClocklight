/*
 * RC5ControlledClocklight.cpp
 *
 * Created: 07.11.2012 19:09:32
 * Author: Alexander Ransmann
 * Description:	Turn on an LED ring behind a clock, with an IR remote.
 *	Measure time between two falling edges:
 *
 *	3000us => startbit
 *	1800us => logical 1
 *	1200us => logical 0
 *	45ms   => break between the command and the repeat of the command
 *
 *	Actually, only the command is compared. The address from the remote is not compared (maybe soon).
 *	Sony SIRCS code!
 *	Atmel ATmega8
 */

// Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Command to be compared
#define RC5_CMD 0x5E

// Signal from TSOP1738 connected to PB0
#define RC5_DDR DDRB
#define RC5_PORT PORTB
#define RC5_IN PB0

// Output to switch LEDs on at PC0
#define LED_DDR DDRC
#define LED_PORT PORTC
#define LED_OUT PC0

// Prototypes
void InitTimer1();
void InitTimer0();
void StartTimer0();
void StopTimer0();

// Interrupt
ISR(TIMER1_CAPT_vect);
ISR(TIMER0_OVF_vect);

// Variables
volatile uint16_t ICRValue;

volatile uint8_t RC5_cmd_val = 0x00;

volatile uint8_t CmdBitNumber = 7;
volatile uint8_t StartBit = 0;

volatile uint8_t CmdDone = 0;
volatile uint8_t CmdMatch = 0;

volatile uint32_t TimerValue = 0;

// Start program
int main(void)
{
	// Timer initialize
	InitTimer1();
	InitTimer0();

	// Set LED pin to output
	LED_DDR = (1 << LED_OUT);

	// Enable interrupts
	sei();

	while(1)
	{
		// Sleep Mode
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();

		// If command exists ... compare
		if (CmdDone == 1)
		{
			// If matches required command
			if (RC5_cmd_val == RC5_CMD)
			{
				// Sleep disablen
				//sleep_disable();

				// Set output to 1
				LED_PORT |= (1 << LED_OUT);
				// Set CmdMatch to 1 ... no new command will be sampled
				CmdMatch = 1;
				// Set TimerValue to zero
				TimerValue = 0;
				// Start counting
				StartTimer0();
			}

			// Reset all values
			RC5_cmd_val = 0x00;
			CmdBitNumber = 7;
			StartBit = 0;
			CmdDone = 0;
		}
	}
}

// Calculate time between two falling edges in the interrupt
ISR(TIMER1_CAPT_vect)
{
	// Set counter to zero (TCNT1 is now in ICR1)
	TCNT1 = 0;

	// Read the Input Capture Registers
	ICRValue = ICR1;

	// go through the bits
	if (CmdDone == 0 && CmdMatch != 1)
	{
		// Wait until startbit detected
		if (StartBit == 0)
		{
			if (ICRValue > 740 && ICRValue < 760)
			{
				StartBit = 1;
			}
		}
		// If startbit is detected, sample new command
		else
		{
			// If time between two falling edges is 1800�s (range: 1760us - 1840us)
			if (ICRValue > 440 && ICRValue < 460)
			{
				// Set bit at this position to 1
				RC5_cmd_val |= (1 << --CmdBitNumber);
			}
			else
			{
				// CmdBitNumber minus 1 -> bit at this position stays 0
				CmdBitNumber--;
			}
			// If 7 bits are sampled, set CmdDone to 1
			if (CmdBitNumber == 0)
			{
				CmdDone = 1;
			}
		}
	}
}

// Turn the LED output off after 5 seconds. Reset Timer1 values
ISR(TIMER0_OVF_vect)
{
	TimerValue++;

	//LED_PORT |= (1 << LED_OUT);

	if (TimerValue == 39063) // 256 * 500ns * 39063 = 5,000064s
	{
		// Toggle LED output off
		LED_PORT &= ~(1 << LED_OUT);

		// Stop Timer0
		StopTimer0();

		// Reset values
		RC5_cmd_val = 0x00;
		CmdBitNumber = 7;
		StartBit = 0;
		CmdDone = 0;
		CmdMatch = 0;

		// Sleep mode
		//sleep_enable();
		//sleep_cpu();
	}
}

// Initialize Timer1
void InitTimer1()
{
	// Falling edge detection | clk/64= 4�s
	TCCR1B |= ((1 << CS11) | (1 << CS10));

	// Input Capture Interrupt enable
	TIMSK |= (1 << TICIE1);

	// Configure input for TSOP1738 signal
	RC5_DDR &= ~(1 << RC5_IN);
}

// Initialize Timer0 for the LED output
void InitTimer0()
{
	// Overflow Interrupt Enable
	TIMSK |= (1 << TOIE0);
}

void StartTimer0()
{
	// Clk/8 => 2MHz = 500ns
	TCCR0 |= (1 << CS01);
}

void StopTimer0()
{
	// Deactivate Timer0
	TCCR0 &= ~(1 << CS01);
}
