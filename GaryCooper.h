#ifndef GaryCooper_h
#define GaryCooper_h

// Debug stuff
#define DEBUGSERIAL	Serial
#define DEBUG_BAUD_RATE	(9600)

//#define	DEBUG_RAW_GPS
#define DEBUG_SUNCALC
#define	DEBUG_DOOR_CONTROLLER
#define DEBUG_LIGHT_CONTROLLER

// Beep on door change?
#define COOPDOOR_CHANGE_BEEPER

// Finally, the data version for tracking the settings
#define GARYCOOPER_DATA_VERSION	(1)

// Utility functions
void debugPrintDoubleTime(double _t, bool _newline = true);

#endif // COOPBOT_H
