//@author Travis Reed
//@author Joe Meis
//@author Aaron Zatorski
//@author Dan Rust
//@author Darren Hushak

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include "util.h"
#include "open_interface.h"
#include "lcd.h"

	
///Main function
/** 
* Initializes all sensors, external items used, etc. Continually waits for serial input, parses input, and then completes command based on that input
*/

int main(void);

/// Receive a line of command via the serial port
/**
* Waits for new line or ten characters to be entered 
*/
void serial_getline();
