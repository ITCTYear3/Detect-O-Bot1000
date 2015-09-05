joystick: serial.o joystick.o
	arm-cortexa8_neon-linux-gnueabihf-gcc -Wall joystick.o serial.o iofunc/libiofunc.a -o joystick 

joystick.o: joystick.c
	arm-cortexa8_neon-linux-gnueabihf-gcc -Wall -c joystick.c

serial.o: serial.c
	arm-cortexa8_neon-linux-gnueabihf-gcc -Wall -c serial.c

clean:
	rm -rf *.o joystick
