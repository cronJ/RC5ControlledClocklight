RC5ControlledClocklight.cpp

Created: 07.11.2012 19:09:32
Author: Alexander Ransmann
Description:

Turn on an LED ring behind a clock, with an IR remote. 
Measure time between two falling edges:

	3000µs => startbit
	1800µs => logical 1
	1200µs => logical 0
	45ms   => break between the command and the repeat of the command

	Actually, only the command is compared. The address from the remote is not compared (maybe soon).
	Sony SIRCS code!
	Atmel ATmega8
