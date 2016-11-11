#include <Arduino.h>
#include <EEPROM.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "GaryCooper.h"
#include "Pins.h"
#include "SunCalc.h"
#include "Telemetry.h"
#include "BeepController.h"
#include "DoorController.h"
#include "LightController.h"

// GPS parser
CGPSParser g_GPSParser;

// Door controller
CDoorController g_doorController;

// Light controller
CLightController g_lightController;


// Beep controller
CBeepController g_beepController(PIN_BEEPER);

// Settings controller
CSaveController g_saveController('C', 'o', 'o', 'p');
bool settingsLoaded = false;

// Sunrise / Sunset calculator
CSunCalc g_sunCalc;

#define MILLIS_PER_SECOND   (1000L)
#define SECONDS_BETWEEN_UPDATES	(60L)

unsigned long g_timer;

void saveSettings()
{
	DEBUG_SERIAL.print(PMS("Save settings... "));

	// Make sure the header is correct
	g_saveController.updateHeader(GARYCOOPER_DATA_VERSION);

	// Rewind so we write the settings in the correct place
	g_saveController.rewind();

	// Save everything
	g_doorController.saveSettings(g_saveController);
	g_lightController.saveSettings(g_saveController);

	DEBUG_SERIAL.println(PMS("complete."));
}

void loadSettings()
{
	DEBUG_SERIAL.print(PMS("Load settings checking header version: "));

	// If the data version is incorrect then we need to update the EEPROM
	// to default settings
	int headerVersion = g_saveController.getDataVersion();
	DEBUG_SERIAL.print(headerVersion);
	DEBUG_SERIAL.print(PMS(" - "));

	if(headerVersion != GARYCOOPER_DATA_VERSION)
	{
		DEBUG_SERIAL.println(PMS("INCORRECT."));

		// Save defaults from object constructors
		DEBUG_SERIAL.println(PMS("Saving default settings."));
		saveSettings();
	}
	else
	{
		DEBUG_SERIAL.println(PMS("CORRECT."));
	}

	DEBUG_SERIAL.println(PMS("Loading settings... "));

	// Make sure we start at the beginning
	g_saveController.rewind();

	g_doorController.loadSettings(g_saveController);
	g_lightController.loadSettings(g_saveController);

	DEBUG_SERIAL.println(PMS("Load settings complete."));
}

void setup()
{
	// Prep debug port
	DEBUG_SERIAL.begin(DEBUG_BAUD_RATE);

	// Prep the GPS port
	GPS_SERIAL.begin(GPS_BAUD_RATE);

	// Prep the telemetry port
	TELEMETRY_SERIAL.begin(TELEMETRY_BAUD_RATE);

	// Setup the door controller
	g_doorController.setup();

	// And the light controller
	g_lightController.setup();

	// Beep to indicate starting the main loop
	g_beepController.setup();
	g_beepController.beep(BEEP_FREQ_INFO, 50, 50, 2);

	// Prep the update timer. This delay
	// allows the GPS to get some data before we start
	// processing more slowly
	g_timer = millis() + (MILLIS_PER_SECOND * 5);
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
	}

	// Let the door controller time its relay
	g_doorController.tick();

	// Let the beep controller run
	g_beepController.tick();

	// Process all available GPS data
	while(GPS_SERIAL.available())
	{
		unsigned char GPSData[256];
		unsigned int GPSDataLen = 0;

		GPSDataLen = GPS_SERIAL.available();
		if(GPSDataLen > sizeof(GPSData - 1))
			GPSDataLen = sizeof(GPSData - 1);

		GPSDataLen = GPS_SERIAL.readBytes((unsigned char *)GPSData, GPSDataLen);
		if(GPSDataLen)
		{
#ifdef DEBUG_RAW_GPS
			GPSData[GPSDataLen] = '\0';
			String rawGPS((const char *)GPSData);
			DEBUG_SERIAL.print(rawGPS);
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

		// Send startup notice
		static bool s_startupTelemetrySent = false;
		if(!s_startupTelemetrySent)
		{
			s_startupTelemetrySent = true;
			telemetrySend(telemetry_tag_startup, TELEMETRY_VALUE_NULL);
		}

		// If the GPS is not sending any data then report an error
		if(!s_gpsDataStreamActive)
		{
			DEBUG_SERIAL.println(PMS("*** NOT RECEIVING ANY DATA FROM GPS. ***"));

			g_GPSParser.getGPSData().clear();
			g_beepController.beep(BEEP_FREQ_ERROR, 100, 50, 1);
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
			g_beepController.beep(BEEP_FREQ_ERROR, 100, 50, 2);
		}
	}
}


bool timeIsBetween(double _currentTime, double _first, double _second)
{
	if(_first < _second)
	{
		if((_currentTime >= _first) && (_currentTime < _second))
			return true;
		else
			return false;
	}
	else
	{
		if(	((_currentTime >= _first) && (_currentTime < 24.)) ||
				((_currentTime > 0.) && (_currentTime < _second)) )
			return true;
		else
			return false;
	}
}

void debugPrintDoubleTime(double _t, bool _newline)
{
	int hour = (int)_t;
	int minute = 60. * (_t - hour);
	DEBUG_SERIAL.print(hour);
	DEBUG_SERIAL.print(PMS(":"));
	DEBUG_SERIAL.print(minute);
	if(_newline) DEBUG_SERIAL.println();
}


