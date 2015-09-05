#ifndef THERMO_H
#define THERMO_H

/* 
	Author: Mark Mahony	 Date: November 15, 2013
 * Purpose: Read temperature values from the thermocouple via the ADC ports on 
 *			a BeagleBone Black.
 * Assumptions: The analog ports have already been set up properly.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define ANALOGPORT "sys/devices/somewhere/AIN2" // Analog port to read from.
#define MVPERCELSIUS 43.4
#define ADCRESOLUTION (1800/4096)   // Resolution of BeagleBone Black ADC.
#define TEMPERATUREBUFSIZE 10   // Number of characters to read.
#define	OFFSET	  900	   // Millivolts offset of temperature reading (900mV = 0C)

int fd = 0;

float readThermocouple () {
	
	char temperatureString [TEMPERATUREBUFSIZE] = {0};  // Contains character string 
														//read from analog device.
	int temperatureRead = 0;	// Converted character string to integer value.
	float calculatedTemperature = 0;  // Final converted temperature in Celsius.	

	if (fd <= 0) {  // Check if analog port has been opened.
		printf ("\nAnalog port not open.\n");
		return (float)0xFFFF;   // If not, return invalid value.
	}
	else {
		read (fd, temperatureString, TEMPERATUREBUFSIZE);   // Read from analog device.
		temperatureRead = atoi (temperatureString); // Convert to numerical value.
	
		// Convert to milivolts.
		calculatedTemperature = (temperatureRead * ADCRESOLUTION) - OFFSET;	
		// Convert to celsius.
		calculatedTemperature /= MVPERCELSIUS;	
		return calculatedTemperature;
	}
	
}

int initThermocouple () {
	if ((fd = open (ANALOGPORT, O_RDONLY)) <= 0) {	// Try to open analog device.
		printf ("\nAnalog device for thermocouple not found\n");   // If not found, tell the user. 
		return -1;
	}
	return 1;
}

void closeThermocouple () {
	if (close (fd) != 0) {
		printf ("\nCould not close thermocouple analog port\n");
	}
}

#endif