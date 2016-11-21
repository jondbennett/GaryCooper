////////////////////////////////////////////////////////////
// Arduino pin assignments
////////////////////////////////////////////////////////////
#ifndef PINS_H
#define PINS_H

// Onboard LED
#define PIN_HEARTBEAT_LED	(LED_BUILTIN)

// Door switches
#define PIN_DOOR_OPEN_SWITCH	(24)	// Door open sensor switch
#define PIN_DOOR_CLOSED_SWITCH	(25)	// Door closed sensor switch

// Relay pins
#define RELAY_ON	(0)
#define RELAY_OFF	(1)

#define	PIN_DOOR_RELAY			(26)	// Door push-button relay
#define PIN_LIGHT_RELAY			(27)	// Light on/off relay

#define PIN_BEEPER				(45)	// Audio Beeper

// Debug serial port
#define DEBUG_SERIAL	Serial
#define DEBUG_BAUD_RATE	(9600)

// Telemetry serial port
#define TELEMETRY_PORT			(1)

// GPS serial port
#define GPS_SERIAL  	Serial2
#define GPS_BAUD_RATE 	(9600)

#endif
