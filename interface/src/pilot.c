#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <curses.h>
#include <signal.h>
#include <math.h>
#include <time.h>

static void finish(void);
void homescreen(void);
void clearscreen(void);
void listcommands(void);
void gotocommandline(void);
void starttimer(void);
void drawtimer(void);
void proximityalert(int y,int x);
void serialerror(int y,int x);
void ackerror(int y,int x);
void finished(int y,int x);
void roverack(int y,int x);
void init_colors(void);
void drawobjects(void);
void drawgrid(void);
int init_serial(char *argv);
void printerror(int err, int y, int x);


//Global Variables

//File descriptor for Bluetooth Communication
int tty_fd;

//Detected Objects struct
struct object {
	int distance;
	int start_angle;
	int end_angle;
};
struct object object_detected[15];
//Master count of objects
int objectcount = 0;

//Command history
struct command {
	char value[100];
};
struct command history[1000];
int history_index = 0;
//Global timer
time_t timer;


///Main function
/* Initializes ncurses, standard messaging and string variables, then goes into a loop
* Within the loop, gets a string from the terminal input (waiting for a line break)
* Interprets that command based on several strcmp functions, then auto-prints the last ten commands
* Finally, redraws the command line and returns the cursor to the command line
* Also draws the time remaining if the timer has been started
* @param File descriptor of bluetooth serial port (i.e. /dev/tty)
*/
int main(int argc, char *argv[]){   
	unsigned char c = 'D';
	int timer_started = 0;
	char str[80];
	char snd[80];
	char msg[80];
	char rcv[80];
	int m;
	for (m=0;m<80;m++){
		rcv[m] = '\0';
	}
	
	int h;
	int angle;
	
    /* initialize your non-curses data structures here */


    (void) signal(SIGINT, finish);  /* arrange interrupts to terminate */
    (void) initscr();      			/* initialize the curses library */
    keypad(stdscr, TRUE);  			/* enable keyboard mapping */
    (void) nonl();         			/* tell curses not to do NL->CR/NL on output */
    (void) cbreak();     			/* take input chars one at a time, no wait for \n */
    (void) echo();         			/* echo input - in color */
    
	init_colors();
	homescreen();

    while(1){
    
 		if (timer_started == 1){
 			drawtimer();	
 		}
		gotocommandline();
 		getstr(str);
        strcpy(history[history_index].value,str);
        
        
        /* process the command keystroke */
        
        
        //Exit
        if (!strcmp(str,"quit")){
        	//Terminate Ncurses and close file descriptor for the serial communication
        	finish(0);
        }
        //Serial init
        else if (!strcmp(str,"connect")){
        	//Initializes serial using the input argument of the whole program
   			int serial = init_serial(argv[1]);
        	switch(serial){
        		case -1:
        			//Serial Error
        			serialerror(LINES/2 - 1,COLS/2 - 11);
        			break;
        		case 0:
        			//Serial connects, but does not receive an ack 'a' char back from the robot
					ackerror(LINES/2 - 1,COLS/2 - 11);
					break;
				case 1:
					//If an 'a' is sent back from the robot, init_serial will pass a 1, indicating good two way connection
					roverack(LINES/2 - 1,COLS/2 - 11);
					break;   	
        		}
        	
        }
        //List Commands
        else if (!strcmp(str,"help")){
        	//Lists all commands
        	listcommands();
        }
        //Start Timer
        else if (!strcmp(str,"start")){
        	//Starts 15 minute timer
        	starttimer();
        	timer_started = 1;
        }
        //Clear screen
        else if (!strcmp(str,"clear")){
        	clearscreen();
        }
        //Scan
        else if (!strncmp(str,"scan", 4)){
        	//Look for input arguments - if less than 6, no arguments, and scan with a default averaging value of 1
        	if (strlen(str) < 6){
        		msg[0] = 's';
        		msg[1] = '1';
        	}
        	//If chars, there is a argument...
        	else if (strlen(str) == 6){
        		//If argument is 'f', then transmit an 'i,' telling the robot to do a fast scan
        		if (str[5] == 'f'){
        			msg[0] = 'i';
        			//This '1' is ignored, but still sent, as three bytes are sent in the send command
        			msg[1] = '1';
        		}
        		else{
        			//If not 'f', then regular scan with argument # of averages per scan
    				msg[0] = 's';
        			msg[1] = str[5];
        		}
        	}
        	//Newline, necessary for robot to receive commands
        	msg[2]='\r';
        	
        	//Clear the active screen
        	clearscreen();
    		
    		//Initialize object detection variables
    		//Object counter
    		objectcount = 0;
    		//Byte to be written to during read
    		char c = 'q';
    		//Timeout check variable
    		int done;
    		int bytesread;
    		//For loop counters
    		int i = 0;
    		int j = 0;
    		int l = 0;
    		//Temporary string to hold ascii integers, later to be converted to actual integer types
    		char intstring[5] = {0,0,0,0,0};
    		//Empty string, to regularly replace the intstring
    		char empty[5] = {0,0,0,0,0};
    		
    		move(10,0);
    		
    		//Reset the rcv string
			int m;
			for (m=0;m<80;m++){
				rcv[m] = '\0';
			}
    		
    		//Send the message to scan
    		write(tty_fd,msg,3);
    		
    		//Timeout reference point (start of loop)
    		time_t t = time(NULL);
    		
    		//Reading loop
    		while(1){
    			//Wait for a byte to be read
    			done = 0;
    			while(!done){
    				bytesread = read(tty_fd,&c,1);
    				//Byte read error
    				if (bytesread<0){
    					break;
    				}
    				//If byte hasn't been read, check for timeout
    				else if (bytesread == 0){
    					if((time(NULL) - t) > 1){
    						rcv[i] = 0;
    						break;
    					}
    				}
    				//If byte has been read, write to rcv, advance rcv index, and break out of the loop
    				else if (bytesread > 0){
    					rcv[i] = c;
    					i++;
    					done =1;
    				}
    			}
    			//Loop timeout (11 seconds), or if it sees a 'z', indicating end of transmission
    			if (c == 'z' || (time(NULL) - t) > 11){
    				break;
    			}
    		}
    		
    		//Parse through whole string (except the last character) to find objects
	   		for(j=0;j<i;j++){
   	 			switch (rcv[j]){
   	 				case 'o':
   	 					//Indicates new object, resets intstring and intstring index
    					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'd':
   	 					//Indicates the past set of numbers were object number, which doesn't mean much here; also resets instring
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 's':
   	 					//Last set of numbers were distance value, in a string of chars (used chars instead of actual values so display would be human readable)
   	 					//Write to object detected struct the integer value of the distance
						object_detected[objectcount].distance=atoi(intstring);
						//Reset intstring
   	 					memcpy(intstring,empty,5);
						l = 0;
   	 					break;
   	 				case 'e':
   	 					//Last set of numbers were angle start value, write to struct and reset intstring
   	 					object_detected[objectcount].start_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'q':
   	 					//End of object, write end angle to struct, reset intstring, advance object count
   	 					object_detected[objectcount].end_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					objectcount++;
   	 					break;
   	 				default:
   	 					//If not a letter, assuming a number has been transmitted, write to intstring for later conversion to actual integer
   	 					intstring[l] = rcv[j];
   	 					l++;
   	 			}
   	 			
   	 		}
   	 		//Write the string received to history, so printscan can use past scans to re-print
   	 		history_index++;
   	 		strcpy(history[history_index].value,rcv);
   	 		//Draw the polar grid
   	 		drawgrid();
   	 		//Draw all of the objects
   	 		drawobjects();
        }
        //Print an old scan
        else if (!strncmp(str,"printscan", 9)){
        	//Handles single, double, or triple digit arguments
        	if (strlen(str) == 11){
        		msg[0] = str[10];
        		msg[1] = 0;
        	}
        	else if (strlen(str) == 12){
        		msg[0] = str[10];
        		msg[1] = str[11];
        		msg[2] = 0;
        	}
        	else if (strlen(str) == 13){
        		msg[0] = str[10];
        		msg[1] = str[11];
        		msg[2] = str[12];
        		msg[3] = 0;
        	}
        	//Converts the argument array of chars to its actual integer
        	int cmd_num = atoi(msg);
        	//Makes sure that the command number is actually in the bounds of the history
        	if (cmd_num < history_index && cmd_num > 0){
        		//Copies the saved string to the rcv string
        		strcpy(rcv,history[cmd_num].value);
        		
        		clearscreen();
    		
    			//Initialize object detection variables
    			//Object counter
    			objectcount = 0;
    			//Byte to be written to during read
   		 		char c = 'q';
    			//Timeout check variable
   		 		int done;
    			int bytesread;
    			//For loop counters
    			int i = 0;
    			int j = 0;
    			int l = 0;
    			//Temporary string to hold ascii integers, later to be converted to actual integer types
    			char intstring[5] = {0,0,0,0,0};
    			//Empty string, to regularly replace the intstring
    			char empty[5] = {0,0,0,0,0};
    			
    			move(10,0);    		
	   			 //Parse through whole string (except the last character) to find objects
	   			for(j=0;j<i;j++){
  	 	 			switch (rcv[j]){
 	  	 				case 'o':
   	 						//Indicates new object, resets intstring and intstring index
    						memcpy(intstring,empty,5);
   	 						l = 0;
   		 					break;
   		 				case 'd':
	   	 					//Indicates the past set of numbers were object number, which doesn't mean much here; also resets instring
   		 					memcpy(intstring,empty,5);
   		 					l = 0;
   		 					break;
   		 				case 's':
   	 						//Last set of numbers were distance value, in a string of chars (used chars instead of actual values so display would be human readable)
   	 						//Write to object detected struct the integer value of the distance
							object_detected[objectcount].distance=atoi(intstring);
							//Reset intstring
   	 						memcpy(intstring,empty,5);
							l = 0;
   	 						break;
   	 					case 'e':
   	 						//Last set of numbers were angle start value, write to struct and reset intstring
   	 						object_detected[objectcount].start_angle=atoi(intstring);
   	 						memcpy(intstring,empty,5);
   	 						l = 0;
   	 						break;
   	 					case 'q':
   	 						//End of object, write end angle to struct, reset intstring, advance object count
   	 						object_detected[objectcount].end_angle=atoi(intstring);
   	 						memcpy(intstring,empty,5);
   	 						l = 0;
   	 						objectcount++;
   	 						break;
   	 					default:
   	 						//If not a letter, assuming a number has been transmitted, write to intstring for later conversion to actual integer
   	 						intstring[l] = rcv[j];
   	 						l++;
   	 				}	
   	 			}
   	 			//Draw the polar grid
   	 			drawgrid();
   	 			//Draw all of the objects
   	 			drawobjects();
   	 		}
        } 
        //Rotate Left         
        else if (!strncmp(str,"left", 4)){
        	//Rotate left with a three digit argument
        	if (strlen(str) == 8){
        		snd[0] = 'l';
        		snd[1] = str[5];
        		snd[2] = str[6];
        		snd[3] = str[7];
        		snd[4] = '\r';
        		snd[5] = '\0';
    			write(tty_fd,snd,5);
        	}
        } 
        //Move forward, displaying errors       
        else if (!strncmp(str,"forward",7)){
        	//Makes sure of correct string length, then packages message to send to robot
        	if (strlen(str) == 11){
        		//f for forward
        		snd[0] = 'f';
        		//3 digits of distance argument
        		snd[1] = str[8];
        		snd[2] = str[9];
        		snd[3] = str[10];
        		//Newline for robot receive parsing
        		snd[4] = '\r';
        		//Null for send
        		snd[5] = '\0';
        		//Send the message
    			write(tty_fd,snd,5);
    			
    			//Wait for input from the serial port
    			while(read(tty_fd,&c,1)!=1){}
    			//If input is not equal to zero...
    			if (c){
    				//Print a proximity alert..
    				proximityalert(LINES/2 - 1,COLS/2 - 11);
    				//And the error message corresponding to the correct error code
					printerror(c,LINES/2 + 2,COLS/2 - 11);
					//Add the error to the command history
					history_index++;
					switch (c){
							case 1:
								strcpy(history[history_index].value,"**   LEFT BUMPER   **");
								break;
							case 2:
								strcpy(history[history_index].value,"**  RIGHT BUMPER   **");
								break;
							case 3:
								strcpy(history[history_index].value,"**    LEFT CLIFF   **");
								break;
							case 4:
								strcpy(history[history_index].value,"**   RIGHT CLIFF   **");
								break;
							case 5:
								strcpy(history[history_index].value,"**FRONT LEFT CLIFF **");
								break;
							case 6:
								strcpy(history[history_index].value,"**FRONT RIGHT CLIFF**");
								break;
							case 7:
								strcpy(history[history_index].value,"** LEFT WHEEL DROP **");
								break;
							case 8:
								strcpy(history[history_index].value,"** RIGHT WHEEL DROP**");
								break;
							case 9:
								strcpy(history[history_index].value,"**   CASTER DROP   **");
								break;
							case 10:
								strcpy(history[history_index].value,"**     IR WALL     **");
								break;
							case 255:
								strcpy(history[history_index].value,"** TARGET REACHED  **");
								break;
						}
    			}
    			//If error == 0, then clear the screen and display a "Command Finished" popup
    			else{
    				clearscreen();
    				finished(LINES/2 - 1,COLS/2 - 11);
    			}
    		}
        }  
        //Reverse, displaying when complete      
        else if (!strncmp(str,"reverse",7)){
        	//Checks for correct input args
        	if (strlen(str) == 11){
        		//b for Back
        		snd[0] = 'b';
        		//Distance arg
        		snd[1] = str[8];
        		snd[2] = str[9];
        		snd[3] = str[10];
        		//Newline, for robot parsing
        		snd[4] = '\r';
        		snd[5] = '\0';
        		//Send message
    			write(tty_fd,snd,5);
    			//Wait for return value
    			while(read(tty_fd,&c,1)!=1){}
    			if (c){
    				//If something other than 0 is returned, print errors **Not generally used, as reverse sends no errors**
    				proximityalert(LINES/2 - 1,COLS/2 - 11);
					printerror(c,LINES/2 + 2,COLS/2 - 11);
    			}
    			else{
    				//Print completed message
    				clearscreen();
    				finished(LINES/2 - 1,COLS/2 - 11);
    			}
    		}
        } 
        //Rotate Right    
        else if (!strncmp(str,"right",5)){
        	if (strlen(str) == 9){
        		snd[0] = 'r';
        		snd[1] = str[6];
        		snd[2] = str[7];
        		snd[3] = str[8];
        		snd[4] = '\r';
        		snd[5] = '\0';
    			write(tty_fd,snd,5);
        	}
        } 
        //Play music              
        else if (!strcmp(str,"victory")){
        		snd[0] = 'm';
        		snd[1] = '\r';
        		snd[2] = '\0';
    			write(tty_fd,snd,3);
        }  
        //Display history as far back as the screen will allow
        else if (!strcmp(str,"history")){
        	//Don't include history command in command history (lolololol)
        	history_index--;
        	for (h=0;h<LINES-5;h++){
			if(history_index-h>=0){
				if (LINES-5-h > 3){
					move(LINES-5-h,3);
					sprintf(msg,"%d: %s      ",history_index - h,history[history_index-h].value);
					addstr(msg);
				}
			}
		}
        }
        //Clear the serial buffer          
        else if (!strcmp(str,"clearbuff")){
        	sleep(2); //required to make flush work, for some reason
  			tcflush(tty_fd,TCIOFLUSH);
        }        

        //Auto-Print History
		for (h=0;h<10;h++){
			if(history_index-h>=0){
				move(LINES-5-h,3);
				sprintf(msg,"%d: %s      ",history_index - h,history[history_index-h].value);
				addstr(msg);
			}
		}
		//Advance history index
        history_index++;
    }
    
	//If loop ever breaks
    finish(0);               /* we're done */
}

///End the program gracefully
static void finish(void){
	//End ncurses window
    endwin();
    //Close the serial port
    close(tty_fd);
	//Exit the program
    exit(0);
}

///Draw the homescreen
void homescreen(void){
	
	
	//Draw the home screen
	attrset(COLOR_PAIR(2));
	int i;
	
	//Header
	move(1,COLS/2-20);
	addstr("TEAM NUMBER ONE :: MARS ROVER INTERFACE");
	move(3,0);
	for (i=0;i<COLS;i++){
		addch('*');
	}
	
	
	//Footer and command input
	move(LINES-2,0);
	for (i=0;i<COLS;i++){
		addch('*');
	}
	
	gotocommandline();	
}

///Clear the active screen
void clearscreen(void){

	int i;
	for(i=4;i<LINES-2;i++){
		//Advance a line
		move(i,0);
		//Clear line
		clrtoeol();
	}
	gotocommandline();	
}

///Start the global 15 minute timer
void starttimer(void){
	//Timer end time (NOW + 15 minutes * 60 seconds)
	timer = time(NULL) + (15*60);
}

///Draw how much time is left in the upper right hand of the screen
void drawtimer(void){
	//Current time
	time_t now = time(NULL);
	//Time remaining
	time_t remain = timer - now;
	//Time message string
	char time[12];
	//Move to upper right hand corner
	move(1,COLS-10);
	//Check time remaining, highlight yellow if less than 10 minutes, red if less than 5, blinking if less than 60
	if (remain<600){
		attrset(COLOR_PAIR(3));
	}
	if (remain<300){
		attrset(COLOR_PAIR(1));
    }
    if (remain < 60){
    	attron(A_BLINK);
    }
    //Print time
    if (remain > 0){
    	sprintf(time,"%02d:%02d    ",(int)remain / 60, (int)remain %60);
    }
    //If time is out, print TIME's UP
     else {
    	sprintf(time,"TIME'S UP");
    }
    //Actually add the string to the screen
	addstr(time);
	//Reset color pair to green on black
    attrset(COLOR_PAIR(2));
}

///List all of the commands available to the user
void listcommands(void){
	
	//List of commands
	
	//Change x and y to move the starting location
	int x=3;
	int y=5;
	move(y,x);
	addstr("LIST OF COMMANDS");
	y++;
	move(y,x);
	addstr("* connect");
	y++;
	move(y,x);
	addstr("* forward ###DIST");
	y++;
	move(y,x);
	addstr("* reverse ###DIST");
	y++;
	move(y,x);
	addstr("* left ###ANGLE");
	y++;
	move(y,x);
	addstr("* right ###ANGLE");
	y++;
	move(y,x);
	addstr("* scan #AVERAGES or f for fast");
	y++;
	move(y,x);
	addstr("* victory");
	y++;
	move(y,x);
	addstr("* history");
	y++;
	move(y,x);
	addstr("* clearbuff");
	y++;
	move(y,x);
	addstr("* start");
	y++;
	move(y,x);
	addstr("* clear");
	y++;
	move(y,x);
	addstr("* help");
	y++;
	move(y,x);
	addstr("* quit");
	gotocommandline();
	
}

///Draws the command line and sets the cursor to the prompt
void gotocommandline(void){
	//Return to command line
	move(LINES-1,0);
	clrtoeol();
	addstr("rover$ ");
}

///Pops up a proximity alert (red)
/*
* @param y cursor location to start the alert
* @param x cursor location to start the alert
*/
void proximityalert(int y,int x){

	attrset(COLOR_PAIR(1));
    move(y,x);
    addstr("-----------------------");
    move(y+1,x);
    addch('|');
    attron(A_BLINK);
    addstr("** PROXIMITY ALERT **");
    attroff(A_BLINK);
    addch('|');
	move(y+2,x);
    addstr("-----------------------");
    attrset(COLOR_PAIR(2));


}

///Pops up with command finished (tan)
/*
* @param y cursor location to start the alert
* @param x cursor location to start the alert
*/
void finished(int y,int x){

	attrset(COLOR_PAIR(7));
    move(y,x);
    addstr("-----------------------");
    move(y+1,x);
    addch('|');
    attron(A_BLINK);
    addstr("**COMMAND COMPLETE!**");
    attroff(A_BLINK);
    addch('|');
	move(y+2,x);
    addstr("-----------------------");
    attrset(COLOR_PAIR(2));


}

///Pops up rover connected (blue)
/*
* @param y cursor location to start the alert
* @param x cursor location to start the alert
*/
void roverack(int y,int x){

    attrset(COLOR_PAIR(5));
    move(y,x);
    addstr("-----------------------");
    move(y+1,x);
    addch('|');
    attron(A_BLINK);
    addstr("** ROVER CONNECTED **");
    attroff(A_BLINK);
    addch('|');
	move(y+2,x);
    addstr("-----------------------");
    attrset(COLOR_PAIR(2));


}

///Pops up with serial error (yellow)
/*
* @param y cursor location to start the alert
* @param x cursor location to start the alert
*/
void serialerror(int y,int x){

	attrset(COLOR_PAIR(3));
    move(y,x);
    addstr("-----------------------");
    move(y+1,x);
    addch('|');
    attron(A_BLINK);
    addstr("**  SERIAL  ERROR  **");
    attroff(A_BLINK);
    addch('|');
	move(y+2,x);
    addstr("-----------------------");
    attrset(COLOR_PAIR(2));


}

///Pops up with ack error (yellow)
/*
* @param y cursor location to start the alert
* @param x cursor location to start the alert
*/
void ackerror(int y,int x){

    attrset(COLOR_PAIR(3));
    move(y,x);
    addstr("-----------------------");
    move(y+1,x);
    addch('|');
    attron(A_BLINK);
    addstr("**    ACK ERROR    **");
    attroff(A_BLINK);
    addch('|');
	move(y+2,x);
    addstr("-----------------------");
    attrset(COLOR_PAIR(2));


}

///Initialize Color pairs
void init_colors(void){
    if (has_colors())
    {
        start_color();

        /*
         * Simple color assignment, often all we need.  Color pair 0 cannot
	 * be redefined.  This example uses the same value for the color
	 * pair as for the foreground color, though of course that is not
	 * necessary:
         */
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_BLACK, 	COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }
}

///Start serial connection
/* Initializes termios session, which is put into a file descriptor, defined globally as tty_fd
* Sets all flags, opens connection
* Sends an 'a,' waits for acknowlegdement 'a' from robot
* @param pointer to the location of the file descriptor for the desired serial port (/dev/tty for example)
* @return an int: -1 for error, 0 for ACK error, 1 for successful ACK
*/
int init_serial(char *argv){

	//Change x and y to move the starting location

   	
	//Serial Communication Initialization
	struct termios tio;
	char c;
	
	//Termios flags set
    memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=10;
    //Open the serial connection, giving a file descriptor (defined globally) 
    tty_fd=open(argv, O_RDWR | O_NONBLOCK);     
    if (tty_fd == -1){
    	//error
       	return -1;
	} 
	
	//Set baud rates
	cfsetospeed(&tio,B57600);  
    cfsetispeed(&tio,B57600);           
    tcsetattr(tty_fd,TCSANOW,&tio);
    //Send an acknowledgement ACK 'a' char to the robot
	write(tty_fd,"a\r",2);
	
	//Wait for return character, should get an 'a' back
	while (1){
		if (read(tty_fd,&c,1)>0){
			if(c=='a'){
				//ACK worked, return 1 for ack correct
				return 1;
			}
			else {
				//Serial connected, but didn't receive an 'a'
				return 0;
			}
		}           
    }
	return 0;
}

///Draw detected objects to screen
/* Uses the detected object global struct
* Passes through all objects, takes their distance, start and end angles, and converts them to pixel locations to print to screen
* Also prints the numerical details (including calculated size) in line form to the screen
*/
void drawobjects(void){
	//String holder
	char str[80];
	//Counter of the extraneous blips
	int outliers = 0;
	//For loop counter
	int i;
	//Momentary pixel positions, calculated for each object interation
	int y;
	int x;
	//Calculated angle center (in radians) of object
	float center;
	//Distance of object in cm
	float distance;
	//Calculated center angle in degrees
	int centerangle;
	//Calculated size in degrees
	float size;
	//Calculated size in centimeters
	float size_cm;
	
	//Draw flashing blue $ to show rover proxy
	move(LINES - 3,COLS/2);
	attrset(COLOR_PAIR(5));
	attron(A_BLINK);
	addch('$');
	
	//Cycle through the objects in the object struct
	for (i=0;i<objectcount;i++){
		//Calculate the center angle from the start and end angles
		centerangle = ((object_detected[i].end_angle + object_detected[i].start_angle)/2);
		//Calculate size in degrees
		size = object_detected[i].end_angle - object_detected[i].start_angle;
		//Calculate center in radians
		center = 0.0174532925*(centerangle);
		//Write distance in cm
		distance = object_detected[i].distance;
		//Calculate size in cm from distance and size
		size_cm = tan((0.0174532925*size)/2)*distance*2;
		//Exclude extraneous blips or errors
		if (distance < 100 && distance > 0 && centerangle > 0 && centerangle <180){
			if (distance<20){
				//If close, draw red
				attrset(COLOR_PAIR(1));
			}
			else{
				//Else, draw yellow
				attrset(COLOR_PAIR(3));
			}
			if (size_cm<6){
				//If les than 6 cm (goalpost), draw blue
				attrset(COLOR_PAIR(5));
			}
			//Calculate pixel locations of each object
			y = LINES - 3 - (((float)distance/100) * (COLS/4) * (float)sin(center));
			x = (COLS/2)*(distance/100)*cos(center) + COLS/2;
			//Move cursor to pixel location
			move(y,x);
			//Draw # and number of object
			sprintf(str,"#%d", i+1);
   	 		addstr(str);
   	 		//Printout actual values
   	 		move (5+i-outliers,1);
   	 		attrset(COLOR_PAIR(2));
			sprintf(str,"#%d, Dist:%.0f Ang:%d Size:%d Size(cm):%.1f Start:%d End:%d", i+1-outliers, distance, centerangle, (int)size/2, size_cm,object_detected[i].start_angle, object_detected[i].end_angle);
   	 		addstr(str);
   	 	}
   	 	//If blip, increment outliers
   	 	else{
   	 		outliers++;
   	 	}
	}
	//Reset color pair
   	attrset(COLOR_PAIR(2));
}

///Draws polar dot grid
void drawgrid(void){
	attrset(COLOR_PAIR(4));
	float center;
	float radian;
	float distance;
	int y;
	int x;
	for (distance=15;distance<=90;distance+=15){
		for (center=0;center<=180;center+=15){
			radian = 0.0174532925 * center;
			y = (float)LINES - 3 - (((float)distance/100) * ((float)COLS/4) * (float)sin(radian));
			x = ((float)COLS/2)*(distance/100)*(float)cos(radian) + (float)COLS/2;
			move((int)y,(int)x);
   	 		addstr(".");
		}
	}
	attrset(COLOR_PAIR(2));
}

///Displays interpreted error
/* @param err error number returned from the robot's forward command
* @param y pixel location to print error message
* @param x pixel location to print error message
*/
void printerror(int err, int y, int x){
	attrset(COLOR_PAIR(1));
    move(y,x);
    addch('|');
   	attron(A_BLINK);
	switch (err){
		case 1:
			addstr("**   LEFT BUMPER   **");
			break;
		case 2:
			addstr("**  RIGHT BUMPER   **");
			break;
		case 3:
			addstr("**    LEFT CLIFF   **");
			break;
		case 4:
			addstr("**   RIGHT CLIFF   **");
			break;
		case 5:
			addstr("**FRONT LEFT CLIFF **");
			break;
		case 6:
			addstr("**FRONT RIGHT CLIFF**");
			break;
		case 7:
			addstr("** LEFT WHEEL DROP **");
			break;
		case 8:
			addstr("** RIGHT WHEEL DROP**");
			break;
		case 9:
			addstr("**   CASTER DROP   **");
			break;
		case 10:
			addstr("**     IR WALL     **");
			break;
		case 255:
			attrset(COLOR_PAIR(2));
			attron(A_BLINK);
			addstr("** TARGET REACHED  **");
			break;
			
	}
	attrset(COLOR_PAIR(1));
    attroff(A_BLINK);
    addch('|');
	move(y+1,x);
    addstr("-----------------------");
 	attrset(COLOR_PAIR(2));
	}