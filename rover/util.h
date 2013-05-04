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
#include "open_interface.h"
#include "lcd.h"

/// Blocks for a specified number of milliseconds
/**
* Uses a prescaled timer to calculate milliseconds
* @param time_val an integer representing how many milliseconds to wait
*/
void wait_ms(unsigned int time_val);

/// Start timer2
/**
* Initializes timer2, used for the wait_ms function
* @param unit 0 for slow, 1 for fast
*/
void timer2_start(char unit);

/// Stop timer2
void timer2_stop();


/// Interrupt handler
/**
* Runs every 1 ms and increments the timer counter
*/
ISR (TIMER2_COMP_vect);

//Push Buttons and Shaft Encoder

/// Initialize PORTC to accept push buttons as input
void init_push_buttons(void);

/// Return the position of button being pushed
/**
 * Return the position of button being pushed.
 * @return the position of the button being pushed.  A 1 is the rightmost button.  0 indicates no button being pressed
 */
char read_push_buttons(void);

/// Initialize PORTC for input from the shaft encoder
void shaft_encoder_init(void);

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
char read_shaft_encoder(void);



//Stepper Motor and Servo

/// Initialize PORTE to control the stepper motor
void stepper_init(void);

/// Turn the Stepper Motor
/**
 * Turn the stepper motor a given number of steps. 
 *
 * @param num_steps A value between 1 and 200 steps (1.8 to 360 degrees)
 * @param direction Indication of direction: 1 for CW and -1 for CCW 
 */
void  move_stepper_motor_by_step(int num_steps, int direction);






//Sonar

///Timer 1 Overflow Counter
ISR (TIMER1_OVF_vect);

///Timer 1 captures a rising edge
ISR (TIMER1_CAPT_vect);

///Initialize Sonar
/**
* Enables timer 1 and timer 1's event interrupt, set 'state' to low
*/
void sonar_init();

///Send a pulse on the Sonar
/**
* Set Pin D to output, send a pulse, wait a millisecond, set the pulse to low, then set PD4 to input, then wait for event interrupt
*/
void pulse();

///Distance Calculator
/**
* Calculates distance from time (34,000 cm/seconds = 34 cm/ microsecond, divided by 16 because the prescalar of 1024 causes the timer to increment every 1/15625 of a second
* We're trying to get microseconds, so divide that 15625 by 1000, so the timer increments every 1/15.6 of a microsecond, which is close to 16, so divide by 16. For some unexplained reason, I had to divide by 2 to get calibration correct.
* @param time Time in an integer value from the ping sensor (from rising edge to falling edge) - represents timer ticks
* @return distance in centimeters
*/
unsigned distancecalc(int time);

///Send a ping and return distance
/**
* Sends a pules, set to low state, wait for done state (handled in the pulse and timer interrupt routines)
* @return distance in centimeters
*/

unsigned ping_read();





//IR Sensor

///Initialize ADC for IR sensor
void IR_init();

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
float line(float x1, float x2, float y1, float y2, float x);

///Distance Calculation from ADC's measured value
/**
* Takes in the raw distance from the ADC's 10 bit value, does linear interpolation in five regions and returns the distance in centimeters
* @param distance_val the raw distance value from the ADC 
* @return distance in centimeters
*/
int distance_lookup(int distance_val);

///Read ADC and return its scaled value
/**
* Reads ADC
* @param channel channel to use for the ADC
* @return distance in centimeters
*/
unsigned ir_read(char channel);




//Servo Control


///Initialize servo
void servo_init( void );

///Set servo to degree angle
/**
* Moves servo to a degree angle
* @param degree angle in degrees to set the servo to
*/
void move_servo(unsigned degree);

///Calculate Servo degree value
/**
* Takes in a degree value, and returns the PWM value required (Top value of the WGM)
* @param degree desired angle value in degrees
* @return WGM top value
*/
unsigned servo_degree_calc(unsigned degree);




//Serial

///Initialize USART0 to a given baud rate
void serial_init(void);

///Receive a character
/**
* Waits for the serial input register is full
* @return char in serial register
*/
char serial_getc();

///Send a character
/**
* Sends a character over the serial port
* @param data character to write to serial port
*/
void serial_putc(char data);

///Send a string
/**
* Loops through a string and uses serial_putc to place each individual char on the serial send
* @param data pointer to a string to send
*/
void serial_putstr(char data[]);



//Movement

///Moves forward by distance cm, 1 is forward. -1 is backwards
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param distance distance to move in centimeters
* @return error value or complete acknowledge
*/
int forward(int distance);

/// Go backwards, ignoring all alerts
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param distance distance to move in centimeters
* @return complete acknowledge
*/
int reverse(int distance);


///Rotate for a specified amount
/**
* Sets wheels forward, constantly checks sensor data for errors
* @param degrees degrees to move
* @param direction 1 for clockwise, -1 for counterclockwise
*/
void rotate(int degrees,int direction);


///Stop
void stop();


///DO DONUTS
void dodonuts();

///Initialize Everything
void init_all();


///Scan/Sonar function
/**
* Loops 0 to 180, setting the servo to that degree setting
* then takes ave_num number of scans, on both ping and IR
* Averages those reads, and writes them to the sensor_reading struct
* After reading is done, munch through the sensor_reading, and detect objects
* After objects have been detected from raw sensor data, format and send over serial
* @param ave_size number of averages to take per degree
*/
void scan(int ave_size);

///Scan/Sonar function
/**
* Loops 0 to 180, setting the servo to that degree setting
* then takes one scan on just the IR and writes it to the sensor_reading struct
* After reading is done, munch through the sensor_reading, and detect objects
* After objects have been detected from raw sensor data, format and send over serial
*/
void scanfast();

///Play music
void playsong(char *notes, char *duration);


///Beep
void beep(void);

///Play a sound on error
void errorsound(void);