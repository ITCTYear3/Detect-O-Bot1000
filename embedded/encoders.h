#ifndef _ENCODERS_H
#define _ENCODERS_H

#define ENC1        1
#define ENC2        2

#define TC_ENC1     0
#define TC_ENC2     1

/* These values have been changed but may need to be re-calculated with a new tracked chassis.   Oct 15, 2013  */ 
#define ENCODER_VANE_DIST       12280    // Distance of one encoder vane in millimeters times 10000 (actual 0.2741mm per vane) (not used)
#define ENCODER_VANE_DIVISOR    10000
#define ENCODER_VANE_METRE      814    // Number of encoder vanes in one metre (calculated value is 3648, but this was found to be a bit low)


/*****************************************************************************/

void encoder_init(void);
word encoder_count(byte encoder);
dword encoder_period(byte encoder);
void motor_set_distance(byte, word);


#endif // _ENCODERS_H