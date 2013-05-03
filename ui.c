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

static void finish(int sig);
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


//Main, takes in location of bluetooth serial port as argument (i.e. /dev/tty)
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
	//char rcv[80] = "o1d45s15e63\no2d75s175e177\n\r";
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
        if (!strcmp(str,"quit")){
        	finish(0);
        }
        else if (!strcmp(str,"connect")){
   			int serial = init_serial(argv[1]);
        	switch(serial){
        		case -1:
        			serialerror(LINES/2 - 1,COLS/2 - 11);
        			break;
        		case 0:
					ackerror(LINES/2 - 1,COLS/2 - 11);
					break;
				case 1:
					roverack(LINES/2 - 1,COLS/2 - 11);
					break;   	
        		}
        	
        }
        else if (!strcmp(str,"help")){
        	listcommands();
        }
        else if (!strcmp(str,"start")){
        	starttimer();
        	timer_started = 1;
        }
        else if (!strcmp(str,"clear")){
        	clearscreen();
        }
        else if (!strncmp(str,"scan", 4)){
        	if (strlen(str) < 6){
        		msg[0] = 's';
        		msg[1] = '1';
        	}
        	else if (strlen(str) == 6){
        		if (str[5] == 'f'){
        			msg[0] = 'i';
        			msg[1] = '1';
        		}
        		else{
    				msg[0] = 's';
        			msg[1] = str[5];
        		}
        	}
        	msg[2]='\r';
        	
        	clearscreen();
    		
    		objectcount = 0;
    		char c = 'q';
    		int done;
    		int bytesread;
    		int i = 0;
    		int j = 0;
    		int l = 0;
    		char intstring[5] = {0,0,0,0,0};
    		char empty[5] = {0,0,0,0,0};
    		move(10,0);
    		
    		
			int m;
			for (m=0;m<80;m++){
				rcv[m] = '\0';
			}
    		//wait for input
    		
    		write(tty_fd,msg,3);
    		time_t t = time(NULL);
    		while(1){
    			done = 0;
    			while(!done){
    				bytesread = read(tty_fd,&c,1);
    				if (bytesread<0){
    					break;
    				}
    				else if (bytesread == 0){
    					if((time(NULL) - t) > 1){
    						rcv[i] = 0;
    						break;
    					}
    				}
    				else if (bytesread > 0){
    					rcv[i] = c;
    					i++;
    					done =1;
    				}
    			}
    			if (c == 'z' || (time(NULL) - t) > 11){
    				break;
    			}
    		}
    		
	   		for(j=0;j<i;j++){
   	 			switch (rcv[j]){
   	 				case 'o':
    					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'd':
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 's':
						object_detected[objectcount].distance=atoi(intstring);
   	 					memcpy(intstring,empty,5);
						l = 0;
   	 					break;
   	 				case 'e':
   	 					object_detected[objectcount].start_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'q':
   	 					object_detected[objectcount].end_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					objectcount++;
   	 					break;
   	 				default:
   	 					intstring[l] = rcv[j];
   	 					l++;
   	 			}
   	 			
   	 		}
   	 		history_index++;
   	 		strcpy(history[history_index].value,rcv);	
   	 		//addstr(rcv);
   	 		drawgrid();
   	 		drawobjects();
        }
        else if (!strncmp(str,"printscan", 9)){
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
        	int cmd_num = atoi(msg);
        	if (cmd_num < history_index && cmd_num > 0){
        		strcpy(rcv,history[cmd_num].value);
        		
        		clearscreen();
    		
    			objectcount = 0;
    			char c = 'q';
    			int done;
    			int bytesread;
    			int i = 0;
    			int j = 0;
    			int l = 0;
    			char intstring[5] = {0,0,0,0,0};
    			char empty[5] = {0,0,0,0,0};
    			move(10,0);    		
	   			for(j=0;j<strlen(rcv);j++){
   	 				switch (rcv[j]){
   	 				case 'o':
    					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'd':
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 's':
						object_detected[objectcount].distance=atoi(intstring);
   	 					memcpy(intstring,empty,5);
						l = 0;
   	 					break;
   	 				case 'e':
   	 					object_detected[objectcount].start_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					break;
   	 				case 'q':
   	 					object_detected[objectcount].end_angle=atoi(intstring);
   	 					memcpy(intstring,empty,5);
   	 					l = 0;
   	 					objectcount++;
   	 					break;
   	 				default:
   	 					intstring[l] = rcv[j];
   	 					l++;
   	 				}
   	 			
   	 			}
   	 			//addstr(rcv);
   	 			drawgrid();
   	 			drawobjects();
   	 		}
        }           
        else if (!strncmp(str,"left", 4)){
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
        else if (!strncmp(str,"forward",7)){
        	if (strlen(str) == 11){
        		snd[0] = 'f';
        		snd[1] = str[8];
        		snd[2] = str[9];
        		snd[3] = str[10];
        		snd[4] = '\r';
        		snd[5] = '\0';
    			write(tty_fd,snd,5);
    			while(read(tty_fd,&c,1)!=1){}
    			if (c){
    				proximityalert(LINES/2 - 1,COLS/2 - 11);
					printerror(c,LINES/2 + 2,COLS/2 - 11);
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
    			else{
    				clearscreen();
    				finished(LINES/2 - 1,COLS/2 - 11);
    			}
    		}
        }        
        else if (!strncmp(str,"reverse",7)){
        	if (strlen(str) == 11){
        		snd[0] = 'b';
        		snd[1] = str[8];
        		snd[2] = str[9];
        		snd[3] = str[10];
        		snd[4] = '\r';
        		snd[5] = '\0';
        		move(5,45);
        		addstr(snd);
    			write(tty_fd,snd,5);
    			while(read(tty_fd,&c,1)!=1){}
    			if (c){
    				proximityalert(LINES/2 - 1,COLS/2 - 11);
					printerror(c,LINES/2 + 2,COLS/2 - 11);
    			}
    			else{
    				clearscreen();
    				finished(LINES/2 - 1,COLS/2 - 11);
    			}
    		}
        }     
        else if (!strncmp(str,"right",5)){
        	if (strlen(str) == 9){
        		snd[0] = 'r';
        		snd[1] = str[6];
        		snd[2] = str[7];
        		snd[3] = str[8];
        		snd[4] = '\r';
        		snd[5] = '\0';
        		move(5,5);
        		addstr(snd);
    			write(tty_fd,snd,5);
        	}
        }               
        else if (!strcmp(str,"victory")){
        		snd[0] = 'm';
        		snd[1] = '\r';
        		snd[2] = '\0';
    			write(tty_fd,snd,3);
        }  
        else if (!strcmp(str,"history")){
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
		
        history_index++;
    }
    

    finish(0);               /* we're done */
}

//End the program gracefully
static void finish(int sig){
    endwin();
    
    close(tty_fd);

    /* do your non-curses wrapup here */

    exit(0);
}

//Draw the homescreen
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

//Clear the active screen
void clearscreen(void){

	int i;
	for(i=4;i<LINES-2;i++){
		move(i,0);
		clrtoeol();
	}
	gotocommandline();	
}

//Start the global 15 minute timer
void starttimer(void){
	timer = time(NULL) + (15*60);
}

//Draw how much time is left in the upper right hand of the screen
void drawtimer(void){
	time_t now = time(NULL);
	time_t remain = timer - now;
	char time[12];
	move(1,COLS-10);
	if (remain<600){
		attrset(COLOR_PAIR(3));
	}
	if (remain<300){
		attrset(COLOR_PAIR(1));
    }
    if (remain < 60){
    	attron(A_BLINK);
    }
    if (remain > 0){
    	sprintf(time,"%02d:%02d    ",(int)remain / 60, (int)remain %60);
    }
    else {
    	sprintf(time,"TIME'S UP");
    }
	addstr(time);
    attrset(COLOR_PAIR(2));
}

//List all of the commands available to the user
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
	addstr("* scan");
	y++;
	move(y,x);
	addstr("* victory");
	y++;
	move(y,x);
	addstr("* history");
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

//Draws the command line and sets the cursor to the prompt
void gotocommandline(void){
	//Return to command line
	move(LINES-1,0);
	clrtoeol();
	addstr("rover$ ");
}

//Pops up a proximity alert
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

//Pops up with command finished
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

//Pops up rover connected
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

//Pops up with serial error
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

//Initialize Color pairs
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

//Start serial connection
int init_serial(char *argv){

	//Change x and y to move the starting location

   	
	//Serial Communication Initialization
	struct termios tio;
	char c;
	    
    memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=10;
        
    tty_fd=open(argv, O_RDWR | O_NONBLOCK);     
    if (tty_fd == -1){
       	return -1;
	} 
	cfsetospeed(&tio,B57600);  
    cfsetispeed(&tio,B57600);           
    tcsetattr(tty_fd,TCSANOW,&tio);
        
	write(tty_fd,"a\r",2);
	
	while (1){
		if (read(tty_fd,&c,1)>0){
			if(c=='a'){
				return 1;
			}
			else {
				return 0;
			}
		}           
    }
	return 0;
}

//Draw detected objects to screen
void drawobjects(void){
	char str[80];
	int outliers = 0;
	move (5,3);
   	addstr(str);
	int i;
	int y;
	int x;
	float center;
	float distance;
	int centerangle;
	float size;
	float size_cm;
	move(LINES - 3,COLS/2);
	attrset(COLOR_PAIR(5));
	attron(A_BLINK);
	addch('$');
	for (i=0;i<objectcount;i++){
		centerangle = ((object_detected[i].end_angle + object_detected[i].start_angle)/2);
		size = object_detected[i].end_angle - object_detected[i].start_angle;
		center = 0.0174532925*(centerangle);
		distance = object_detected[i].distance;
		size_cm = tan((0.0174532925*size)/2)*distance*2;
		if (distance < 100 && distance > 0 && centerangle > 0 && centerangle <180){
			if (distance<20){
				attrset(COLOR_PAIR(1));
			}
			else{
				attrset(COLOR_PAIR(3));
			}
			if (size_cm<6){
				attrset(COLOR_PAIR(5));
			}
			y = LINES - 3 - (((float)distance/100) * (COLS/4) * (float)sin(center));
			x = (COLS/2)*(distance/100)*cos(center) + COLS/2;
			move(y,x);
			sprintf(str,"#%d", i+1);
   	 		addstr(str);
   	 		move (5+i-outliers,1);
   	 		attrset(COLOR_PAIR(2));
			sprintf(str,"#%d, Dist:%.0f Ang:%d Size:%d Size(cm):%.1f Start:%d End:%d", i+1-outliers, distance, centerangle, (int)size/2, size_cm,object_detected[i].start_angle, object_detected[i].end_angle);
   	 		addstr(str);
   	 	}
   	 	else{
   	 		outliers++;
   	 	}
	}
   	attrset(COLOR_PAIR(2));
}

//Draws grid
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