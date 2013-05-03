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


int rcv[10];
	
///Main function
/** 
* Initializes all sensors, external items used, etc. Continually waits for serial input, parses input, and then completes command based on that input
*/

int main(void)
{
    init_all();
	
	unsigned char notes[26]  = {72, 67, 69, 67,  0, 72, 67, 69, 67,  0, 72, 72, 72, 72,  0, 72, 72, 72, 72,  0, 72, 71, 72, 71, 72};
	unsigned char duration[26]={64, 16, 16, 16, 40, 64, 16, 16, 16, 40, 8,   8, 16, 16, 16, 8,   8, 16, 16, 16, 20, 20, 32, 20, 96};
	
	int error = 0;
	char msg[180];
	char command[10];
	int magnitude = 0;
	int j = 0;
	int averages = 1;
	beep();
	
	
	//Ping Distance Calibration Testing
	
	/*int pingdist;
	int irdist;
	while(1){		
		wait_ms(200);
		pingdist = ping_read();
		irdist = ir_read(2);
		lprintf("PING: %d\nIR  : %d\nAVE : %d",pingdist,irdist,(pingdist+irdist)/2);
	}*/
	
	while(1){
		
		//lprintf("Awaiting commands");
		error = 0;
		j = 0;
		
		//Wait for a new line from the serial port
		serial_getline();
		beep();
		
		//Check commands
		switch (rcv[0]){
			case 'a':
				//Connect acknowledgement
				serial_putc('a');
				lprintf("********************\nRover Connected!\n********************");
				break;
			case 'f':
				//Forward with a three digit argument
				command[0] = rcv[1];
				command[1] = rcv[2];
				command[2] = rcv[3];
				command[3] = '\0';
				//Convert ASCII to an integer
				magnitude = atoi(command);
				//Move forward, returning error data from the sensors
				error = forward(magnitude);
				lprintf("Forward");
				//Return the error to the host
				serial_putc((char)error);
				break;			
			case 'b':
				//Backward with a three digit argument - ignores sensor data
				command[0] = rcv[1];
				command[1] = rcv[2];
				command[2] = rcv[3];
				command[3] = '\0';
				//Convert ASCII to integer
				magnitude = atoi(command);
				//Move backwards, return a complete ack
				error = reverse(magnitude);
				lprintf("Backward");
				//Send ack
				serial_putc((char)error);
				break;
			case 'r':
				//Move right by a three digit argument
				command[0] = rcv[1];
				command[1] = rcv[2];
				command[2] = rcv[3];
				command[3] = '\0';
				magnitude = atoi(command);
				lprintf("Rotate Right: %d",magnitude	);
				rotate(magnitude,1);
				break;
			case 'l':
				//Move left by a three digit argument
				command[0] = rcv[1];
				command[1] = rcv[2];
				command[2] = rcv[3];
				command[3] = '\0';
				magnitude = atoi(command);  
				lprintf("Rotate Left: %d",magnitude);
				rotate(magnitude,-1);
				break;	
			case 's':
				//Scan, return objects (within the scan function)
				command [0] = rcv[1];
				command [1] = '\0';
				//Take in an arg to determine how many averages per degree the scanner uses
				averages = atoi(command);
				lprintf("Scanning\n%d Averages", averages);
				//Scan
				scan(averages);
				beep();
				break;	
			case 'i':
				//Fast scan, only uses the IR sensor
				lprintf("Scanning fast");
				scanfast();
				beep();
				break;
			case 'm':
				//Music, for completion of the landing spot
				playsong(notes, duration);
				//dodonuts();
				break;	
		}
		
		if(error){
			//Print error, make a noise if there is an error
			sprintf(msg,"Error #: %d", error);
			lprintf(msg);
			errorsound();
		}	
		
		//Reset the rcv string to be all 0's
		for(int i = 0; i < strlen(rcv); i++){
			rcv[i] = 0;
		}	
	}
}

/// Receive a line of command via the serial port
/**
* Waits for new line or ten characters to be entered 
*/
void serial_getline(){
	
	//String index initialization
	int j = 0;
	
	//Wait until the previous character is a new line, or 10 chars have been received
	while((rcv[j-1] != 13) && (j<10)){
		
		//Write to rcv string
		rcv[j] = serial_getc();
		j++;
	}
	//Write null character to final bit of rcv string
	rcv[j] = 0;
}
