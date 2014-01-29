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

// Global used for interrupt driven delay functions
volatile unsigned int timer2_tick;
void timer2_start(char unit);
void timer2_stop();


/// Blocks for a specified number of milliseconds
/**
* Uses a prescaled timer to calculate milliseconds
* @param time_val an integer representing how many milliseconds to wait
*/
void wait_ms(unsigned int time_val) {
	//Seting OC value for time requested
	OCR2=250; 				//Clock is 16 MHz. At a prescaler of 64, 250 timer ticks = 1ms.
	timer2_tick=0;
	timer2_start(0);

	//Waiting for time
	while(timer2_tick < time_val);

	timer2_stop();
}

/// Start timer2
/**
* Initializes timer2, used for the wait_ms function
* @param unit 0 for slow, 1 for fast
*/
void timer2_start(char unit){
	timer2_tick=0;
	if ( unit == 0 ) { 		//unit = 0 is for slow 
        TCCR2=0b00001011;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 64
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
	}		
	if (unit == 1){ 		//unit = 1 is for fast
        TCCR2=0b00001001;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 1
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
		}
	sei();
}

/// Stop timer2
void timer2_stop() {
	TIMSK&=~0b10000000;		//Disabling O.C. Interrupt for Timer2
	TCCR2&=0b01111111;		//Clearing O.C. settings
}


/// Interrupt handler
/**
* Runs every 1 ms and increments the timer counter
*/
ISR (TIMER2_COMP_vect) {
	timer2_tick++;
}

//Push Buttons and Shaft Encoder

/// Initialize PORTC to accept push buttons as input
void init_push_buttons(void) {
	DDRC &= 0xC0;  //Setting PC0-PC5 to input
	PORTC |= 0x3F; //Setting pins' pull up resistors
}

/// Return the position of button being pushed
/**
 * Return the position of button being pushed.
 * @return the position of the button being pushed.  A 1 is the rightmost button.  0 indicates no button being pressed
 */
char read_push_buttons(void) {
	char i;
	for(i=0;i<6;i++)
	{
		int j = 5 - i;
		if (((PINC >> j) & 1) == 0){
			return j + 1;
		}
	}
	return 0;
}

/// Initialize PORTC for input from the shaft encoder
void shaft_encoder_init(void) {
	DDRC &= 0x3F;	//Setting PC6-PC7 to input
	PORTC |= 0xC0;	//Setting pins' pull-up resistors
}

/// Read the shaft encoder
/**
 * Reads the two switches of the shaft encoder and compares the values
 * to the previous read.  This function should be called very frequently
 * for the best results.
 *
 * @return a value indicating the shaft encoder has moved:
 * 0 = no rotation (switches did not change)
 * 1 = CW rotation
 * -1 = CCW rotation
 */
char read_shaft_encoder(void) {
	
	// static variable to store the old value of A and B.
	// This variable will only be initialized the first time you call this function.
	static char old_value = 192;		// Step 2: Based on how you decided to mask PINC in step 1, what would new_value be if the switch is in a groove?
	
	// Function variables
	char new_value = PINC & 0b11000000;			// Step 1: Decide how to read PINC so that the push buttons are masked
	char rotation = 0;

	// If the knob was in a groove
	if (old_value == 192) {		// Step 3: Enter here what you did for step 2.
		if (new_value == 64)		// What will new_value be if you're rotating CW out of a groove
		rotation = 1;
		if (new_value == 128)		// What will new_value be if you're rotating CCW out of a groove
		rotation = -1;
	}	
	old_value = new_value;

	return rotation;
}



//Stepper Motor and Servo

/// Initialize PORTE to control the stepper motor
void stepper_init(void) {
	DDRE |= 0xF0;  	//Setting PE4-PE7 to output
	PORTE &= 0x8F;  //Initial postion (0b1000) PE4-PE7
	wait_ms(2);
	PORTE &= 0x0F;  //Clear PE4-PE7
}

/// Turn the Stepper Motor
/**
 * Turn the stepper motor a given number of steps. 
 *
 * @param num_steps A value between 1 and 200 steps (1.8 to 360 degrees)
 * @param direction Indication of direction: 1 for CW and -1 for CCW 
 */
void  move_stepper_motor_by_step(int num_steps, int direction) {
	static char state=0b00010000;
	
	int i=0;
	if (direction == 1){
		for(i=0;i<num_steps;i++){
			PORTE = (PORTE & 0x0F);
			if (state){
				state = state << 1;
			}
			else{
				state = 0x10;
			}
			PORTE = (PORTE | state);
			wait_ms(15);
		}
	}
	else if(direction == 0){
		
	}
	else {
		for(i=0;i<num_steps;i++){
			PORTE = (PORTE & 0x0F);
			if (state >> 1 & 0xF0){
				state = state >> 1;
			}
			else{
				state = 0b10000000;
			}
			PORTE = (PORTE | state);
			wait_ms(15);
		}
		
	}
	

	
	PORTE = (PORTE & 0x0F);	
}






//Sonar

volatile unsigned rising_time; // start time of the return pulse
volatile unsigned falling_time; // end time of the return pulse
volatile int overflow=0; //Overflow Counter
volatile int state; //Ping state - 0 is LOW, 1 is HIGH, 2 is DONE

///Timer 1 Overflow Counter
ISR (TIMER1_OVF_vect)
{
	//Overflow events
	overflow++;
}

///Timer 1 captures a rising edge
ISR (TIMER1_CAPT_vect)
{
	//If in low state, then set to high state and set ICES to read
	//If in high state, set to done
	switch (state) {
		case 0:
		rising_time = ICR1; // save captured time
		TCCR1B &= ~_BV(ICES); // to detect falling edge
		state = 1; // now in HIGH state
		break;

		case 1:
		falling_time = ICR1; // save captured time
		TCCR1B |= _BV(ICES); // to detect falling edge
		state = 2; // now it’s DONE
		break;
		default:
		break;
	}
}

///Initialize Sonar
/**
* Enables timer 1 and timer 1's event interrupt, set 'state' to low
*/
void sonar_init()
{
	// enable Timer1 and interrupt
	// Set TCCR1B: Rising Edge sets trigger, Noice Canceller, no Waveform Generator, prescalar 1024 (CS=101)
	TCCR1B = 0b11000101;
	//Enable Input Catpure Interrupt, and Overflow Interrupt
	TIMSK |= 0b00100100;
	//Initialize State to LOW
	state=0;
}

///Send a pulse on the Sonar
/**
* Set Pin D to output, send a pulse, wait a millisecond, set the pulse to low, then set PD4 to input, then wait for event interrupt
*/
void pulse()
{
	DDRD	|=	0x10; // set PD4 as output
	PORTD	&=	~0x10; // set PD4 to low
	PORTD	|=	0x10; // set PD4 to high
	wait_ms(1); // wait
	PORTD	&=	~0x10; // set PD4 to low
	DDRD	&=	~0x10; // set PD4 as input
}

///Distance Calculator
/**
* Calculates distance from time (34,000 cm/seconds = 34 cm/ microsecond, divided by 16 because the prescalar of 1024 causes the timer to increment every 1/15625 of a second
* We're trying to get microseconds, so divide that 15625 by 1000, so the timer increments every 1/15.6 of a microsecond, which is close to 16, so divide by 16. For some unexplained reason, I had to divide by 2 to get calibration correct.
* @param time Time in an integer value from the ping sensor (from rising edge to falling edge) - represents timer ticks
* @return distance in centimeters
*/
unsigned distancecalc(int time)
{
	return (time*34/32);
}

///Send a ping and return distance
/**
* Sends a pules, set to low state, wait for done state (handled in the pulse and timer interrupt routines)
* @return distance in centimeters
*/

unsigned ping_read()
{
	pulse(); // send the starting pulse to PING
	state = 0; // now in the LOW state
	while (state != 2){} // wait until IC is done
	return distancecalc(falling_time - rising_time); // calculate and return distance
}





//IR Sensor

///Initialize ADC for IR sensor
void IR_init()
{
	// REFS=11, ADLAR=0, MUX don’t care
	ADMUX = _BV(REFS1) | _BV(REFS0);
	// ADEN=1, ADFR=0, ADIE=0, ADSP=111
	// others don’t care
	// See page 246 of user guide
	ADCSRA = _BV(ADEN) | (7<<ADPS0);
}

///Linear interpolation
/**
* Takes in two points and an x value, returns a y value along that line
* @param x1 float representing the x value of point one on a line
* @param x2 float representing the x value of point two on a line
* @param y1 float representing the y value of point one on a line
* @param y2 float representing the y value of point two on a line
* @param x float representing the x value that the desired calculated y value is paired with
* @return float representing the point on the line that the x input corresponds to
*/
float line(float x1, float x2, float y1, float y2, float x){
	float y;
	float slope = (y2-y1)/(x2-x1);
	float intercept = y1 - slope*x1;
	y = (slope*x + intercept);
	//y = (int)y;
	return y;
	
}

///Distance Calculation from ADC's measured value
/**
* Takes in the raw distance from the ADC's 10 bit value, does linear interpolation in five regions and returns the distance in centimeters
* @param distance_val the raw distance value from the ADC 
* @return distance in centimeters
*/
int distance_lookup(int distance_val){
	int distance;
	if (distance_val>710){
		distance = line(1000,710,10,15,distance_val);
		return distance;
	}
	else if (distance_val>500){
		distance = line(710,500,15,20,distance_val);
		return distance;
	}
	else if (distance_val>290){
		distance = line(500,290,20,40,distance_val);
		return distance;
	}
	else if (distance_val>220){
		distance = line(290,220,40,60,distance_val);
		return distance;
	}
	else{
		distance = line(220,180,60,80,distance_val);
		return distance;
	}
	
};

///Read ADC and return its scaled value
/**
* Reads ADC
* @param channel channel to use for the ADC
* @return distance in centimeters
*/
unsigned ir_read(char channel)
{
	ADMUX |= (channel & 0x1F);
	ADCSRA |= _BV(ADSC);
	while (ADCSRA & _BV(ADSC))
		{}
	return distance_lookup(ADC);
}




//Servo Control

volatile int period = 43000; 	// Pulse frequency, Hz
volatile int mode = 1;
volatile int angle = 0;

///Initialize servo
void servo_init( void )
{
	OCR3A = period-1; 		// number of cycles in the interval
	OCR3B = 800; 			// initial value
	TCCR3A = 0b00100011; 	// set COM and WGM (bits 3 and 2) TCCR3A: [COM3A1,COM3A0,COM3B1,COM3B0,COM3C1,COM3C0,WGM31,WGM30]
	TCCR3B = 0b00011010; 	// set WGM (bits 1 and 0) and CS prescaler set to 8
	TCCR3C = 0b00000000; 	// set FOC3B
	DDRE = 0xFF;			// set Port E pin 4 (OC3B) as output
}

///Set servo to degree angle
/**
* Moves servo to a degree angle
* @param degree angle in degrees to set the servo to
*/
void move_servo(unsigned degree)
{
	OCR3B = servo_degree_calc(degree); // set pulse width
	wait_ms(5);
}

///Calculate Servo degree value
/**
* Takes in a degree value, and returns the PWM value required (Top value of the WGM)
* @param degree desired angle value in degrees
* @return WGM top value
*/
unsigned servo_degree_calc(unsigned degree){
	return (degree * 19.7) + 800;
}




//Serial

///Initialize USART0 to a given baud rate
void serial_init(void) {
	unsigned int baud = 34;// look up in table for correct value
	/* Set baud rate */
	UBRR0H = (unsigned char) (baud >> 8);
	UBRR0L = (unsigned char)baud;
	/* Enable double speed mode */
	UCSR0A = 0b00000010;
	/* Set frame format: 8 data bits, 2 stop bits */
	UCSR0C = 0b00010110;
	/* Enable receiver and transmitter */
	UCSR0B = 0b00011000;
	//UCSR0B |= 0b10000000; // optional: receive interrupt enable bit
}

///Receive a character
/**
* Waits for the serial input register is full
* @return char in serial register
*/
char serial_getc() {
	/* Wait for the receive complete flag (RXC) */
	while ((UCSR0A & 0b10000000) == 0)
	;
	/* Reads data from the receive buffer; clears the receive buffer */
	return UDR0;
}

///Send a character
/**
* Sends a character over the serial port
* @param data character to write to serial port
*/
void serial_putc(char data) {
	/* Wait for empty transmit buffer by checking the UDRE bit */
	while ((UCSR0A & 0b00100000) == 0);
	/* Put data into transmit buffer; sends the data */
	UDR0 = data;
}

///Send a string
/**
* Loops through a string and uses serial_putc to place each individual char on the serial send
* @param data pointer to a string to send
*/
void serial_putstr(char data[]){
	int len = strlen(data);
	int i;
	for (i=0;i<len;i++){
		serial_putc(data[i]);
	}
}



//Movement

///Moves forward by distance cm, 1 is forward. -1 is backwards
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param distance distance to move in centimeters
* @return error value or complete acknowledge
*/
int forward(int distance) {
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	
	int ret =0;
	

	int sum = 0;
	
	oi_set_wheels(200, 200); //
	
	while (sum < distance) {
		oi_update(sensor_data);
		//Check ALLLLLLL of the sensor data - returns an integer relating to the sensor data
		if(sensor_data->bumper_left){
			ret = 1;
			break;
		}
		else if(sensor_data->bumper_right){
			ret = 2;
			break;
		}
		else if(sensor_data->cliff_left){
			ret = 3;
			break;
		}
		else if(sensor_data->cliff_right){
			ret = 4;
			break;
		}			
		else if(sensor_data->cliff_frontleft){
			ret = 5;
			break;
		}		
		else if(sensor_data->cliff_frontright){
			ret = 6;
			break;
		}		
		else if(sensor_data->wheeldrop_left){
			ret = 7;
			break;
		}
		else if(sensor_data->wheeldrop_right){
			ret = 8;
			break;
		}
		else if(sensor_data->wheeldrop_caster){
			ret = 9;
			break;
		}	
		else if(sensor_data->virtual_wall){
			ret = 10;
			break;
		}				
		//BOT 13 cliff left = 400, cliff right = 600, cliff fright = 600, cliff fleft = 500
		//Cliff sensors
		if( sensor_data->cliff_left_signal > 400 && sensor_data->cliff_right_signal > 600 && sensor_data->cliff_frontright_signal > 600 && sensor_data->cliff_frontleft_signal > 500  ){
			lprintf("left: %d\nright: %d\nfrontleft: %d\nfrontright: %d",sensor_data->cliff_left_signal, sensor_data->cliff_left_signal, sensor_data->cliff_frontleft_signal, sensor_data->cliff_frontright_signal);
			ret = 255;
			break;
		}
		//Distance Counter
		sum += abs(sensor_data->distance);
		wait_ms(10);
	}
	stop();
	
	oi_free(sensor_data);
	return ret;
}

/// Go backwards, ignoring all alerts
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param distance distance to move in centimeters
* @return complete acknowledge
*/
int reverse(int distance) {
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	
	int ret =0;

	int sum = 0;
	
	oi_set_wheels(-200, -200); //
	
	while (sum < distance) {
		oi_update(sensor_data);
		sum += abs(sensor_data->distance);
		wait_ms(10);
	}
	stop();
	
	oi_free(sensor_data);
	return ret;
}


///Rotate for a specified amount
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param degrees degrees to move
* @param direction 1 for clockwise, -1 for counterclockwise
*/
void rotate(int degrees,int direction) {
	if(direction>0){
		direction=1;
	}
	else
	{
		direction=-1;
	}
	//makes input valid regardless
	
	//Allocates and intializes sensor data
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	
	//Rotational Counter
	int sum = 0;
	//Turn in a direction
	oi_set_wheels(-150*direction, 150*direction);
	while (sum < degrees) {
		oi_update(sensor_data);
		sum += abs(sensor_data->angle);
		wait_ms(5);
	}
	stop();
	
	oi_free(sensor_data);
}


///Stop
void stop(){
	oi_set_wheels(0, 0);
}


///DO DONUTS
void dodonuts(){
	rotate(720,1);
}

///Initialize Everything
void init_all(){
	lcd_init();
	servo_init();
	init_push_buttons();
	shaft_encoder_init();
	sonar_init();
	stepper_init();
	IR_init();
	serial_init();
}


///Scan/Sonar function
/**
* Loops 0 to 180, setting the servo to that degree setting
* then takes ave_num number of scans, on both ping and IR
* Averages those reads, and writes them to the sensor_reading struct
* After reading is done, munch through the sensor_reading, and detect objects
* After objects have been detected from raw sensor data, format and send over serial
* @param ave_size number of averages to take per degree
*/
void scan(int ave_size)
{
	move_servo(0);
	wait_ms(700);
	int ping_distance=0;
	int ir_distance=0;
	int i;
	//Object loop counter
	int j;
	//Object counter
	int currentObject = 0;
	//Message string
	char str[200];
	
	//Sensor Data storage
	struct sensor_reading{
		int angle_reading;
		int ping_reading;
		int ir_reading;
		int object;
	};
	
	//Data on objects detected
	struct object {
		int distance;
		int start_angle;
		int end_angle;
		int size;
	};
	
	struct sensor_reading sensor_data[200];
	struct object object_detected[15];
	
	int angle = 0;
	int distance_index = 0;
	
	//Starting line for serial printing
	//serial_putstr("\r\n\n\n***********************************\r\nDegrees\t\tIR Distance\tSonar Distance\tObject");
	
	//Move servo by 1 degree every tick
	for (angle=0; angle<180; angle++){
		move_servo(angle);
		wait_ms(10);
		
		ping_distance = 0;
		ir_distance = 0;
		//Average Ping and IR Reads by taking 4 readings
		for (i=0;i<ave_size;i++)
		{
			ping_distance += ping_read();
			ir_distance += ir_read(2);
			wait_ms(1);
		}
		ping_distance = ping_distance/ave_size;
		ir_distance = ir_distance/ave_size;
		
		//Write angle, ping, and IR distance to the sensor data struct
		sensor_data[angle].angle_reading=angle;
		sensor_data[angle].ping_reading=ping_distance;
		sensor_data[angle].ir_reading=ir_distance;
		sensor_data[angle].object = -1;
	}
	
	for(j=1;j<180;j++){
		if((sensor_data[j].ir_reading<90) && (sensor_data[j].ping_reading < 90)){
			//New object
			if(sensor_data[j-1].object != currentObject){
				//Increment object counter
				currentObject++;
				//Write object index into sensor data
				sensor_data[j].object = currentObject;
				//Set the object_detected start angle
				object_detected[currentObject].start_angle = j;
			}
			//Same object
			else {
				//Writes the object index to the sensor data
				sensor_data[j].object = currentObject;
			}
		}
		else{
			//Check the previous scan, see if it was the current object
			if (sensor_data[j-1].object == currentObject){
				//Set the end angle
				object_detected[currentObject].end_angle = j-1;
				//Set the angle size
				object_detected[currentObject].size = object_detected[currentObject].end_angle - object_detected[currentObject].start_angle;
				//Calculate the angle/index of the center of the object
				distance_index = (object_detected[currentObject].start_angle + object_detected[currentObject].end_angle)/2;
				//Record the distance
				object_detected[currentObject].distance = (sensor_data[distance_index].ir_reading + sensor_data[distance_index].ping_reading)/2;
				if (object_detected[currentObject].size  > 2){
					//Formate and send the data via serial
					sprintf(str,"o%id%is%ie%iq",currentObject, object_detected[currentObject].distance, object_detected[currentObject].start_angle, object_detected[currentObject].end_angle);
					serial_putstr(str);
				}					
			}	
		}			
	}

	//Send a z character to indicate end of transmission
	serial_putstr("z");
	
}

///Scan/Sonar function
/**
* Loops 0 to 180, setting the servo to that degree setting
* then takes one scan on just the IR and writes it to the sensor_reading struct
* After reading is done, munch through the sensor_reading, and detect objects
* After objects have been detected from raw sensor data, format and send over serial
*/
void scanfast()
{
	move_servo(0);
	wait_ms(700);
	int ir_distance=0;
	int i;
	//Object loop counter
	int j;
	//Object counter
	int currentObject = 0;
	//Message string
	char str[200];
	
	//Sensor Data storage
	struct sensor_reading{
		int angle_reading;
		int ir_reading;
		int object;
	};
	
	//Data on objects detected
	struct object {
		int distance;
		int start_angle;
		int end_angle;
		int size;
	};
	
	struct sensor_reading sensor_data[200];
	struct object object_detected[15];
	
	int angle = 0;
	int distance_index = 0;
	
	//Starting line for serial printing
	//serial_putstr("\r\n\n\n***********************************\r\nDegrees\t\tIR Distance\tSonar Distance\tObject");
	
	//Move servo by 1 degree every tick
	for (angle=0; angle<180; angle++){
		move_servo(angle);
		wait_ms(5);
		ir_distance = ir_read(2);
		
		//Write angle, ping, and IR distance to the sensor data struct
		sensor_data[angle].angle_reading=angle;
		sensor_data[angle].ir_reading=ir_distance;
		sensor_data[angle].object = -1;
	}
	
	for(j=1;j<180;j++){
		if((sensor_data[j].ir_reading<150)){
			//New object
			if(sensor_data[j-1].object != currentObject){
				//Increment object counter
				currentObject++;
				//Write object index into sensor data
				sensor_data[j].object = currentObject;
				//Set the object_detected start angle
				object_detected[currentObject].start_angle = j;
			}
			//Same object
			else {
				//Writes the object index to the sensor data
				sensor_data[j].object = currentObject;
			}
		}
		else{
			//Check the previous scan, see if it was the current object
			if (sensor_data[j-1].object == currentObject){
				//Set the end angle
				object_detected[currentObject].end_angle = j-1;
				//Set the angle size
				object_detected[currentObject].size = object_detected[currentObject].end_angle - object_detected[currentObject].start_angle;
				//Calculate the angle/index of the center of the object
				distance_index = (object_detected[currentObject].start_angle + object_detected[currentObject].end_angle)/2;
				//Record the distance
				object_detected[currentObject].distance = (sensor_data[distance_index].ir_reading);
				if (object_detected[currentObject].size  > 2){
					//Formate and send the data via serial
					sprintf(str,"o%id%is%ie%iq",currentObject, object_detected[currentObject].distance, object_detected[currentObject].start_angle, object_detected[currentObject].end_angle);
					serial_putstr(str);
					lprintf(str);
				}
			}
		}
	}

	//Send a z character to indicate end of transmission
	serial_putstr("z");
	
}

///Play music
void playsong(char *notes, char *duration){
	forward(0);
	oi_load_song(0,26,notes,duration);
	oi_play_song(0);
}

///Beep
void beep(void){
	oi_t* sensor = oi_alloc();
	oi_init(sensor);
	unsigned char beep_note[1] = {70};
	unsigned char beep_dur[1] = {5};
	oi_load_song(2,1,beep_note,beep_dur);
	oi_play_song(2);
	oi_free(sensor);
}

///Play a sound on error
void errorsound(void){
	oi_t* sensor = oi_alloc();
	oi_init(sensor);
	unsigned char beep_note[3] = {63,72,81};
	unsigned char beep_dur[3] = {5,5,5};
	oi_load_song(3,3,beep_note,beep_dur);
	oi_play_song(3);
	oi_free(sensor);
}