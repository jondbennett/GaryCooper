////////////////////////////////////////////////////
// Configuration file for the program
////////////////////////////////////////////////////
#ifndef GaryCooper_h
#define GaryCooper_h

////////////////////////////////////
// Config settings for Gary Cooper

// Universal constant
#define MILLIS_PER_SECOND   (1000L)

//#define DEBUG_RAW_GPS
#define DEBUG_SUNCALC
#define DEBUG_DOOR_CONTROLLER
#define DEBUG_DOOR_MOTOR
#define DEBUG_LIGHT_CONTROLLER
#define DEBUG_COMMAND_PROCESSOR
//#define DEBUG_COMMAND_PROCESSOR_INTERFACE
#define DEBUG_SETTINGS

// Beep on door change?
#define COOPDOOR_CHANGE_BEEPER

// Beep to report errors?
#define BEEP_ON_ERROR

// The data version for tracking the settings,
// and the settings functions
#define GARYCOOPER_DATA_VERSION	(2)
extern void loadSettings();
extern void saveSettings(bool _defaults = false);

// Important globals
extern CTelemetry g_telemetry;
extern CDoorController g_doorController;
extern CLightController g_lightController;
extern CBeepController g_beepController;
extern CSunCalc g_sunCalc;
extern CSaveController g_saveController;

// Utility functions
void debugPrintDoubleTime(double _t, bool _newline = true);

void reportError(telemetryErrorE _errorTag, bool _set);
void sendErrors();
#endif
