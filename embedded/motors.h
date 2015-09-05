/* Motor macros */
#ifndef _MOTORS_H
#define _MOTORS_H

#include "utils.h"


// Motor direction ports
#define MOTOR_PORT          PORTB
#define MOTOR_PORT_DDR      DDRB

#define MOTOR1_DIRA         PORTB_BIT0
#define MOTOR1_DIRB         PORTB_BIT1
#define MOTOR2_DIRA         PORTB_BIT2
#define MOTOR2_DIRB         PORTB_BIT3
#define MOTOR1_DIRA_MASK    PORTB_BIT0_MASK
#define MOTOR1_DIRB_MASK    PORTB_BIT1_MASK
#define MOTOR2_DIRA_MASK    PORTB_BIT2_MASK
#define MOTOR2_DIRB_MASK    PORTB_BIT3_MASK

#define MOTOR_BITS          ( MOTOR1_DIRA_MASK | MOTOR1_DIRB_MASK | MOTOR2_DIRA_MASK | MOTOR2_DIRB_MASK )
#define MOTOR1_BITS         ( MOTOR1_DIRA_MASK | MOTOR1_DIRB_MASK )
#define MOTOR2_BITS         ( MOTOR2_DIRA_MASK | MOTOR2_DIRB_MASK )


// Motor PWM channels
#define MOTOR1_PWM      4
#define MOTOR2_PWM      5

// Motor Control Law Timer channel
#define TC_MOTOR        5


// Motor numbers
#define MOTOR1          1
#define MOTOR2          2

// Motor numbers in ASCII
#define MOTOR1C        '0'
#define MOTOR2C        '1'

// Motor directions (must be binary opposites!)
// These depend on the orientation of the motor header plugs
#define MOTOR_FW        0
#define MOTOR_RV        1

// We may look into setting this value to 250 or 255 to get a greater resolution for values. October 15, 2013
// Motor default period
#define MOTOR_PER_DEF   100     // 0 - 100%

// It is possible that we could increase this delay as precision is not as much of a requirement. October 15, 2013 
// Motor default control law delta
#define MOTOR_CNTL_LAW_DELTA    30000   // Time in microseconds between each run of the control law


/* Control Law defines */

// These eventually may require tuning
// Initial gain values
#define P_GAIN_DEF      1000
#define I_GAIN_DEF      100

#define MAXDRIVE        100     // Maximum drive value
#define DRIVE_SCALE_VAL 1000000 // Motor drive value scaling
#define GAIN_DIV        1000    // Gain divisor

// Compile time option to choose between 12V and 24V supply
#define TWELVEVOLT
// Should look at deleting 24V values.
//#define TWENTYFOURVOLT    // ALL DEFINES ARE NOT SETUP YET FOR 24V!

#ifdef  TWELVEVOLT     // The following values were obtained in an experiment at 15.6V, from the battery.

#define MINDRIVE1   25      // Minimum drive value at which the motor still move (slowest movement) 
#define MINDRIVE2   23      // Minimum drive value at which the motor still move (slowest movement) 
#define MINPERIOD1  26786   // Minimum encoder period in microseconds for motor 1 (RHS). Only used in precompiler for MAXFREQ  
#define MAXPERIOD1  360000  // Maximum encoder period in microseconds for motor 1 (RHS). Only used in precompiler for MINFREQ 
#define MINPERIOD2  26690   // Minimum encoder period in microseconds for motor 2 (LHS). Only used in precompiler for MAXFREQ 
#define MAXPERIOD2  365750  // Maximum encoder period in microseconds for motor 2 (LHS). Only used in precompiler for MINFREQ 
#define MAXSPEED    337     // OLD VALUE! Maximum speed of left motor in mm/s. Only used for accurate speed movements
#define MINSPEED    0       // OLD VALUE! Minimum speed of left motor in mm/s. Only used for accurate speed movements

#elif   TWENTYFOURVOLT

/* These values need to be looked at and possibly updated.   Oct 15, 2013  */ 
#error "All defines are not setup yet for 24V! These current values are old and need to be updated (2013-10-15)"
#define MINDRIVE    0       // Minimum drive value to overcome detent torque from a stopped position
#define MINPERIOD   370     // Minimum encoder period in microseconds for motor X. Only used in precompiler for MAXFREQ     
#define MAXPERIOD   800     // Maximum encoder period in microseconds for motor X. Only used in precompiler for MINFREQ
   
#else
#error "Either TWELVEVOLT or TWENTYFOURVOLT must be defined"
#endif

#define MAXFREQ1     (DRIVE_SCALE_VAL / MINPERIOD1)   // Maximum encoder frequency in kHz
#define MINFREQ1     (DRIVE_SCALE_VAL / MAXPERIOD1)   // Minimum encoder frequency in kHz
#define BVALUE1      MINDRIVE1 -(((DRIVE_SCALE_VAL * (MINDRIVE1 - MAXDRIVE)) / (MINFREQ1 - MAXFREQ1)) / MAXPERIOD1)
#define MAXFREQ2     (DRIVE_SCALE_VAL / MINPERIOD2)   // Maximum encoder frequency in kHz
#define MINFREQ2     (DRIVE_SCALE_VAL / MAXPERIOD2)   // Minimum encoder frequency in kHz
#define BVALUE2      MINDRIVE2 -(((DRIVE_SCALE_VAL * (MINDRIVE2 - MAXDRIVE)) / (MINFREQ2 - MAXFREQ2)) / MAXPERIOD2)

#define MAX_SPEED_ERROR     MAXDRIVE    // Limits for error that remain in the range of reality 
#define SPEED_SET_SCALE     100         // Used to adjust speed setpoint

#define SPEED_RATIO         297 //(((MAXDRIVE - MINDRIVE) / (MAXSPEED - MINSPEED)) * 1000) Precompiler does not like this calculation, do it manually
#define SPEED_RATIO_DIVISOR 1000
#define SPEED_OFFSET        0

/*****************************************************************************/

void motor_init(void);
void motor_set_direction(byte, byte);
void motor_set_speed(byte, char);
char motor_convert(byte, int);

static void motor_set_period(byte, byte);
static void motor_set_duty(byte, byte);

int abs(int);

extern long speed_error1, speed_error2, read_period1, read_period2, intermediate_drive_value1, intermediate_drive_value2;
extern byte drive_value1, drive_value2;


#endif // _MOTORS_H