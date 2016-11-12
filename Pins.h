
////////////////////////////////////////////////////////////
// Arduino pin assignments
////////////////////////////////////////////////////////////
#ifndef PINS_H
#define PINS_H

// Onboard LED
#define PIN_DOOR_STATE_LED	(LED_BUILTIN)

// Audio Beeper
#define PIN_BEEPER			(45)

// Relay pins
#define	PIN_DOOR_RELAY		(2)
#define PIN_LIGHT_RELAY		(3)

// Debug serial port
#define DEBUG_SERIAL	Serial
#define DEBUG_BAUD_RATE	(9600)

// GPS serial port
#define GPS_SERIAL  	Serial1
#define GPS_BAUD_RATE 	(9600)

// Telemetry serial port
#define TELEMETRY_PORT			(2)
#define TELEMETRY_BAUD_RATE		(115200)


#endif
