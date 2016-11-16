
////////////////////////////////////////////////////////////
// Arduino pin assignments
////////////////////////////////////////////////////////////
#ifndef PINS_H
#define PINS_H

// Onboard LED
#define PIN_HEARTBEAT_LED	(LED_BUILTIN)

// Relay pins
#define	PIN_DOOR_RELAY			(2)		// Door push-button relay
#define PIN_LIGHT_RELAY			(3)		// Light on/off relay
#define PIN_DOOR_OPEN_SWITCH	(5)		// Door open sensor switch
#define PINT_DOOR_CLOSED_SWITCH	(6)		// Door closed sensor switch
#define PIN_BEEPER				(45)	// Audio Beeper

// Debug serial port
#define DEBUG_SERIAL	Serial
#define DEBUG_BAUD_RATE	(9600)

// Telemetry serial port
#define TELEMETRY_PORT			(1)
#define TELEMETRY_BAUD_RATE		(115200)

// GPS serial port
#define GPS_SERIAL  	Serial2
#define GPS_BAUD_RATE 	(9600)

#endif
