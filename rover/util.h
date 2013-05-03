
#include <avr/io.h>
#include <avr/interrupt.h>
#include "open_interface.h"
#include "lcd.h"

/// Blocks for a specified number of milliseconds
void wait_ms(unsigned int time_val);

/// Shaft encoder initialization
void shaft_encoder_init(void);

/// Shaft encoder read function
char read_shaft_encoder(void);

/// Initialize Stepper Motor
void stepper_init(void);

/// Stepper motor move function
void move_stepper_motor_by_step(int num_steps, int direction);

/// Initialize PORTC, which is used by the push buttons
void init_push_buttons(void);

/// Return the position of button being pushed
/**
 * Return the position of button being pushed.
 * @return the position of the button being pushed.  A 1 is the rightmost button.  0 indicates no button being pressed
 */
char read_push_buttons(void);



//Sonar

//Initialize Sonar
void sonar_init();

//Send a pulse on the Sonar
void pulse();

//Distance Calculator
unsigned distancecalc(int time);

//Send a ping and return distance
unsigned ping_read();



//IR Sensor

//Initialize ADC for IR sensor
void IR_init();

//Linear interpolation
float line(float x1, float x2, float y1, float y2, float x);

//Distance Calculation from ADC's measured value
int distance_lookup(int distance_val);

//Read ADC and return its digital scaled value
unsigned ir_read(char channel);



//Servo Control

//Initialize timer 3
void servo_init( void );

//Set servo to degree angle
void move_servo(unsigned degree);

//Calculate Servo degree value
unsigned servo_degree_calc(unsigned degree);

//Move servo by scaler degrees in whichever direction it currently is in
void range_check_and_move(unsigned scaler);

//Serial

//Initialize USART0 to a given baud rate
void serial_init(void);

//Receive a character
char serial_getc();

//Send a character
void serial_putc(char data);

//Send a string
void serial_putstr(char data[]);

//Movement

//Moves forward by distance cm, 1 is forward. -1 is backwards
int forward(int distance);
int reverse(int distance);


//degrees is simply degree of rotation desired
//direction is 1 for clockwise, -1 for counterclockwise
void rotate(int degrees,int direction);


//Stop
void stop();

//Check to see if we're on the target
int checktarget();

//Do donuts
void dodonuts();

//Initialize Everything
void init_all();

//Scan function
void scan(int ave_size);

//Scan fast function - uses only the IR and only one scan per angle
void scanfast();

//Play music
void playsong(char *notes, char *duration);

//Beep
void beep();

//Play a sound on error
void errorsound();