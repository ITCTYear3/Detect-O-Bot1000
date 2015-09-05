/* Detect-O-Bot 1000 joystick for Linux host */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
//#include <pthread.h>
#include "serial.h"		// for serial comms to HCS12 board

#include "iofunc/iolib.h"	// for beaglebone gpio

#include <linux/joystick.h>	// for joystick

#include "thermocouple.h"	// temperature functions

#define NAME_LENGTH 	128

#define CAMTHRESHOLD	3000
#define TURNTHRESHOLD	5
#define TRIGTHRESHOLD	1000
#define CMDDELAYHI		50000000
#define CMDDELAYLO		10000000

#define MONITORTIMEOUT	1000000000

// Speed conversion defines
#define SPEEDMNUM		100
#define	SPEEDMDEN		65534

#define BUFSIZE			8

// strings
#define GAMEPAD			"/dev/input/js0"
#define WEBCAMIMAGE		"vlc -I dummy v4l2:///dev/video1 --video-filter scene --no-audio --scene-path ~/MobileCameraPlatform --scene-prefix image --scene-format ppm vlc://quit --run-time=1"
#define WEBCAMAUDIO		"arecord -f cd -d 5 audio.wav"
#define SERIALPORT		"/dev/ttyO4"

// make sure controller is set to XInput not DirectInput
// Gamepad buttons
#define BUTTONA			0
#define BUTTONB			1
#define BUTTONX			2
#define BUTTONY			3
#define BUTTONLB		4
#define BUTTONRB		5
#define	BUTTONBACK		6
#define BUTTONSTART		7
#define BUTTONLOGI		8
#define BUTTONLHAT		9
#define BUTTONRHAT		10

// Gamepad axes
#define	AXISLTLR		0
#define AXISLTUD		1
#define AXISRTLR		2
#define AXISRTUD		3
#define AXISRTRIG		4
#define AXISLTRIG		5
#define AXISDPADLR		6
#define AXISDPADUD		7

// Beaglebone pins
#define MARKERMOTOR		8, 12
#define MARKERSENSE		9, 12

#define MARKERDELAY		200000000

void *MonitorPlatform(void *arg);

int main (int argc, char **argv)
{

	int *axis;
	int *button;
	struct js_event js;
	int fd;

	int buttonAPressed = 0, buttonBPressed = 0, buttonXPressed = 0, buttonYPressed = 0, buttonRHATPressed = 0,
		buttonLBPressed = 0, buttonRBPressed = 0, buttonSTARTPressed = 0, buttonBACKPressed = 0;

	int aborted = 0;

	struct timespec prevTime, currTime, markerCurrTime, markerPrevTime;
	clockid_t clock;

	int cmdDelay = CMDDELAYHI;
	int leftMotor, rightMotor, speed, oldLeft = 0, oldRight = 0;

	//pthread_t monitorThread;
	//pthread_attr_t attr;

	unsigned char axes = 2;
	unsigned char buttons = 2;
	int version = 0x000800;
	char name[NAME_LENGTH] = "Unknown";

	char buffer[BUFSIZE] = {'0','1','2','3','4','5','6','7'};

	// open the gamepad port
	if ((fd = open(GAMEPAD, O_RDONLY)) < 0) {
		perror("Failed opening gamepad port!");
		exit(1);
	}

	// get info from gamepad driver
	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGAXES, &axes);
	ioctl(fd, JSIOCGBUTTONS, &buttons);
	ioctl(fd, JSIOCGNAME(NAME_LENGTH), name);

	printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
		name, axes, buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);

	// allocate memory for structures
	axis = calloc(axes, sizeof(int));
	button = calloc(buttons, sizeof(char));

	// open and initialize the serial port
    // if first argument is given, override the default serial port
	if (argc == 2) {
		SerialOpen(argv[1]);
		SerialInit(argv[1]);
	} else {
		SerialOpen(SERIALPORT);
		SerialInit(SERIALPORT);
	}

	// open the gamepad port in nonblocking mode
	fcntl(fd, F_SETFL, O_NONBLOCK);

	printf("Running ... (interrupt to exit)\n");

	// initialize and set the thread attributes
	/*
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
	pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );

	if(pthread_create(&monitorThread, &attr, MonitorPlatform, NULL) != 0){
		printf("Error creating monitor thread!\n Exiting...\n");
		exit(11);
	}
	*/

	printf("Initializing beaglebone library\n");

	// initialize beaglebone library
	iolib_init();

	printf("Setting up beaglebone gpio pins\n");

	// set up beaglebone pins 
	iolib_setdir(MARKERMOTOR, DIR_OUT);
	pin_low(MARKERMOTOR);
	iolib_setdir(MARKERSENSE, DIR_IN);

	initThermocouple ();

	// get process clock timer
	clock_getcpuclockid(0, &clock);
	// get current time and set prevTime
	clock_gettime(clock, &prevTime);

	// main loop
	// does a nonblocking read of the gamepad
	// if we get an event, fill out the structures
	// checks the timer
	// if time is right
	// send commands to platform
	while (1) {
		
		// check the timer to see if 200 ms have elapsed since we started the motor
		if(is_high(MARKERMOTOR)){
			clock_gettime(clock, &markerCurrTime);
			if(!(((unsigned)markerCurrTime.tv_nsec - (unsigned)markerPrevTime.tv_nsec) < MARKERDELAY)){
				if(is_low(MARKERSENSE)){
					pin_low(MARKERMOTOR);
				}
			}
		}

		// do a read of the gamepad port
		if(read(fd, &js, sizeof(struct js_event)) == sizeof(struct js_event)){

			// get the type of event, fill out button or axis appropriately
			switch(js.type & ~JS_EVENT_INIT) {
			case JS_EVENT_BUTTON:
				button[js.number] = js.value;
				break;
			case JS_EVENT_AXIS:
				axis[js.number] = js.value;
				break;
			}
		// didn't get an event, check the error to make sure nothing went wrong
		}else if (errno != EAGAIN) {
			perror("\njoystick: error reading");
			exit (1);
		}

		// get current time
		//clock_getcpuclockid(0, &clock);
		clock_gettime(clock, &currTime);

		// if some amount of time has passed, use the current structure
		// to send commands to the camera/platform
		if(!(((unsigned)currTime.tv_nsec - (unsigned)prevTime.tv_nsec) < cmdDelay)){
			
// *********************** A Button ************************************
			if (button[BUTTONA]) {
				if (buttonAPressed == 0) {
					// turn on the marker motor
					printf("Marking location...\n");
					pin_high(MARKERMOTOR);
					clock_gettime(clock, &markerPrevTime);
					buttonAPressed = 1;
				}
			} else {
				buttonAPressed = 0;
			}

// ************************ B Button ***********************************
			if (button[BUTTONB]) {
				if (buttonBPressed == 0) {
					printf("Full Stop...\n");
					snprintf(buffer, BUFSIZE+1, "mov20000");
					SerialWrite((unsigned char *)buffer,BUFSIZE);
					buttonBPressed = 1;
				}
			} else {
				buttonBPressed = 0;
			}

// *********************** X Button ************************************
			if (button[BUTTONX]) {
				// ping the robot
				if (buttonXPressed == 0) {
					printf("Pinging...\n");
					snprintf(buffer, BUFSIZE+1, "png00000");
					SerialWrite((unsigned char *)buffer,BUFSIZE);
					buttonXPressed = 1;
				}
			} else {
				buttonXPressed = 0;
			}

// ********************* Y Button **************************************
			if (button[BUTTONY]) {
				if (buttonYPressed == 0) {
					printf("Full Speed Ahead...\n");
					snprintf(buffer, BUFSIZE+1, "mov20100");
					SerialWrite((unsigned char *)buffer,BUFSIZE);
					buttonYPressed = 1;
				}
			} else {
				buttonYPressed = 0;
			}

// ****************** Left Button **************************************
			if (button[BUTTONLB]) {
				// toggle command delay
				if (buttonLBPressed == 0) {
					if (cmdDelay == CMDDELAYHI) {
						printf("Low command delay\n");
						cmdDelay = CMDDELAYLO;
					} else {
						printf("High command delay\n");
						cmdDelay = CMDDELAYHI;
					}
					buttonLBPressed = 1;
				}
			} else {
				buttonLBPressed = 0;
			}

// ********************** Right Button *********************************
			if (button[BUTTONRB]) {
				// get temperature
				if (buttonRBPressed == 0) {
					printf("Temperature: %.2f\n", readThermocouple());
					buttonRBPressed = 1;
				}
			} else {
				buttonRBPressed = 0;
			}

// ********************** Right Hat ************************************
			if (button[BUTTONRHAT]) {
				// toggle laser sight
				if (buttonRHATPressed == 0) {
					printf("Toggling laser sight...\n");
					snprintf(buffer, BUFSIZE+1, "aim00000");
					SerialWrite((unsigned char *)buffer, BUFSIZE);
					buttonRHATPressed = 1;
				}
			} else {
				buttonRHATPressed = 0;
			}

// ********************** Start Button *********************************
			if (button[BUTTONSTART]) {
				// only resume if aborted first
				if (aborted) {
					if (buttonSTARTPressed == 0) {
						printf("Resuming...\n");
						snprintf(buffer, BUFSIZE+1, "res00000");
						SerialWrite((unsigned char *)buffer,BUFSIZE);
						aborted = 0;
						buttonSTARTPressed = 1;
					}
				}
			} else {
				buttonSTARTPressed = 0;
			}

// ********************** Back Button *********************************
			if (button[BUTTONBACK]) {
				// abort
				if (buttonBACKPressed == 0) {
					printf("Aborting...\n");
					snprintf(buffer, BUFSIZE+1, "abt00000");
					SerialWrite((unsigned char *)buffer,BUFSIZE);
					aborted = 1;
					buttonBACKPressed = 1;
				}
			} else {
				buttonBACKPressed = 0;
			}

// ********************* Right Thumbstick ******************************
			if(axis[AXISRTLR] > CAMTHRESHOLD){
			}else if(axis[AXISRTLR] < -CAMTHRESHOLD){
			}
			
			if(axis[AXISRTUD] > CAMTHRESHOLD){
			}else if(axis[AXISRTUD] < -CAMTHRESHOLD){
			}

// ************************ Speed Updates ******************************

			// convert trigger values into speed
			if(abs(axis[AXISRTRIG] - axis[AXISLTRIG]) < TRIGTHRESHOLD)
				speed = 0;
			else
				speed = (SPEEDMNUM * (axis[AXISRTRIG] - axis[AXISLTRIG])) / SPEEDMDEN;

			// turn left
			if(axis[AXISLTLR] < -TURNTHRESHOLD){
				leftMotor = speed * (100 + ((SPEEDMNUM * axis[AXISLTLR]) / 32767)) / 100;
				rightMotor = speed;
			// turn right
			}else if(axis[AXISLTLR] > TURNTHRESHOLD){
				rightMotor = speed * (100 - ((SPEEDMNUM * axis[AXISLTLR]) / 32767)) / 100;
				leftMotor = speed;
			// go straight
			}else{
				rightMotor = speed;
				leftMotor = speed;
			}

			// check D-Pad for spin
			// spin left
			if(axis[AXISDPADLR] < 0){
				leftMotor = -speed;
				rightMotor = speed;
			// spin right
			}else if(axis[AXISDPADLR] > 0){
				leftMotor = speed;
				rightMotor = -speed;
			}

			// only send drive commands if the motor speed has changed
			if(leftMotor != oldLeft){
				// send commands to drive both motors
				// left motor
				snprintf(buffer, BUFSIZE+1, "mov0%4d", leftMotor);
				SerialWrite((unsigned char *)buffer,BUFSIZE);
				oldLeft = leftMotor;
				printf("%s\n", buffer);
			}
			if(rightMotor != oldRight){
				// right motor
				snprintf(buffer, BUFSIZE+1, "mov1%4d", rightMotor);
				SerialWrite((unsigned char *)buffer,BUFSIZE);
				oldRight = rightMotor;
				printf("%s\n", buffer);
			}

			// get the current time and put it in prevTime
			clock_gettime(clock, &prevTime);
		}

		fflush(stdout);
	}
}

// Never worked, so commented out
/*

// monitors the serial connection for platform response
void * MonitorPlatform(void *arg){
	unsigned char monitorBuffer[8] = {0};
	struct timespec monitorPrevTime, monitorCurrTime;
	int numBytes = 0;
	clockid_t monitorClock;

	// get process clock timer
	clock_getcpuclockid(0, &monitorClock);
	// get current time and set prevTime
	clock_gettime(monitorClock, &monitorPrevTime);


	while(1){
		numBytes = SerialRead(monitorBuffer, 8);

		if(numBytes > 0){
			printf("%s\n", monitorBuffer);

			// get the current time
			clock_gettime(monitorClock, &monitorCurrTime);

			if(((unsigned)monitorCurrTime.tv_nsec - (unsigned)monitorPrevTime.tv_nsec) > MONITORTIMEOUT){
				printf("Large delay between pings detected - Platform error!\n");
			}
		}
	}

	pthread_exit(NULL);
}
*/
