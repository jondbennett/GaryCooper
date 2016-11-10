#include <Arduino.h>
#include <EEPROM.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "GaryCooper.h"
#include "Pins.h"
#include "SlidingBuf.h"
#include "Comm_Arduino.h"
#include "SunCalc.h"
#include "Telemetry.h"
#include "BeepController.h"
#include "DoorController.h"
#include "LightController.h"

// Door Controller
CDoorController g_doorController;

// Light Controller
CLightController g_lightController;

// GPS comm link and parser
#define GPS_SERIAL_PORT  (1)
#define GPS_BAUD_RATE (9600)
CComm_Arduino g_GPSComm;
CGPSParser g_GPSParser;

CComm_Arduino g_telemetryComm;

// Beeper
CBeepController g_beepController(PIN_BEEPER);

// Settings controller
CSaveController g_saveController('C', 'o', 'o', 'p');
bool settingsLoaded = false;

// Sunrise / Sunset calculator
CSunCalc g_sunCalc;

#define MILLIS_PER_SECOND   (1000L)
#define SECONDS_BETWEEN_UPDATES	(60L)
//#define SECONDS_BETWEEN_UPDATES	(5L)

unsigned long g_timer;

void saveSettings()
{
	DEBUGSERIAL.print(PMS("Save settings... "));

	// Make sure the header is correct
	g_saveController.updateHeader(GARYCOOPER_DATA_VERSION);

	// Rewind so we write the settings in the correct place
	g_saveController.rewind();

	// Save everything
	g_doorController.saveSettings(g_saveController);
	g_lightController.saveSettings(g_saveController);

	DEBUGSERIAL.println(PMS("complete."));
}

void loadSettings()
{
	DEBUGSERIAL.print(PMS("Load settings checking header version: "));

	// If the data version is incorrect then we need to update the EEPROM
	// to default settings
	int headerVersion = g_saveController.getDataVersion();
	DEBUGSERIAL.print(headerVersion);
	DEBUGSERIAL.print(PMS(" - "));

	if(headerVersion != GARYCOOPER_DATA_VERSION)
	{
		DEBUGSERIAL.println(PMS("INCORRECT."));

		// Save defaults from object constructors
		DEBUGSERIAL.println(PMS("Saving default settings."));
		saveSettings();
	}
	else
	{
		DEBUGSERIAL.println(PMS("CORRECT."));
	}

	DEBUGSERIAL.println(PMS("Loading settings... "));

	// Make sure we start at the beginning
	g_saveController.rewind();

	g_doorController.loadSettings(g_saveController);
	g_lightController.loadSettings(g_saveController);

	DEBUGSERIAL.println(PMS("Load settings complete."));
}

void setup()
{
	// Prep debug port
	DEBUGSERIAL.begin(DEBUG_BAUD_RATE);

	// Prep telemetry
	telemetrySetup();

	// Prep the GPS comm port
	g_GPSComm.open(GPS_SERIAL_PORT, GPS_BAUD_RATE);

	// Setup the door controller
	g_doorController.setup();

	// And the light controller
	g_lightController.setup();

	// Beep to indicate starting the main loop
	g_beepController.setup();
	g_beepController.beep(BEEP_FREQ, 50, 50, 2);

	// Prep the update timer. This delay
	// allows the GPS to get some data before we start
	// processing more slowly
	g_timer = millis() + (MILLIS_PER_SECOND * 2);
}

void loop()
{
	// Monitor for GPS data flow
	static bool s_gpsDataStreamActive = false;

	// Load settings?
	if(!settingsLoaded)
	{
		loadSettings();
		settingsLoaded = true;
		telemetrySend(telemetry_tag_startup, TELEMETRY_VALUE_NULL);
	}

	// Let the door controller time its relay
	g_doorController.tick();

	// Let the beep controller run
	g_beepController.tick();

	// Cycle the GPS comm link
	g_GPSComm.tick();

	// Process all available GPS data
	while(g_GPSComm.bytesInReceiveBuffer())
	{
		unsigned char GPSData[256];
		unsigned int GPSDataLen = 0;

		GPSDataLen = g_GPSComm.read((unsigned char *)GPSData, sizeof(GPSData - 1));
		if(GPSDataLen)
		{
#ifdef DEBUG_RAW_GPS
			GPSData[GPSDataLen] = '\0';
			String rawGPS((const char *)GPSData);
			DEBUGSERIAL.print(rawGPS);
#endif

			g_GPSParser.parse(GPSData, GPSDataLen);
			s_gpsDataStreamActive = true;
		}
	}

	// Update processing
	if(millis() >= g_timer)
	{
		// Prep for next update
		g_timer = millis() + (MILLIS_PER_SECOND * SECONDS_BETWEEN_UPDATES);

		// If the GPS is not sending any data then report an error
		if(!s_gpsDataStreamActive)
		{
			DEBUGSERIAL.println(PMS("*** NOT RECEIVING ANY DATA FROM GPS. ***"));

			g_GPSParser.getGPSData().clear();
			g_beepController.beep(BEEP_FREQ, 100, 50, 3);
			telemetrySend(telemetry_tag_error, telemetry_error_GPS_no_data);
		}
		s_gpsDataStreamActive = false;

		// If we have a valid time fix, control the door and light
		if(g_sunCalc.processGPSData(g_GPSParser.getGPSData()))
		{
			g_doorController.checkTime();
			g_lightController.checkTime();
		}
		else
		{
			g_beepController.beep(BEEP_FREQ, 100, 50, 4);
		}
	}
}

void debugPrintDoubleTime(double _t, bool _newline)
{
	int hour = (int)_t;
	int minute = 60. * (_t - hour);
	DEBUGSERIAL.print(hour);
	DEBUGSERIAL.print(PMS(":"));
	DEBUGSERIAL.print(minute);
	if(_newline) DEBUGSERIAL.println();
}


