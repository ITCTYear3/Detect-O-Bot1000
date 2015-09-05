#ifndef _SERIAL_H
#define _SERIAL_H

//********************** Prototypes ****************************
void SerialInit(const char serialPort[]);
int SerialOpen(const char serialPort[]);
void SerialClose(void);
int SerialWrite(unsigned char * buffer, int numBytes);
int SerialRead(unsigned char * buffer, int numBytes);

#endif
