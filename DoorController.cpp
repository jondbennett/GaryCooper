////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "GaryCooper.h"
#include "Pins.h"
#include "SunCalc.h"
#include "Telemetry.h"
#include "BeepController.h"
#include "DoorController.h"

extern CBeepController g_beepController;
extern CSunCalc g_sunCalc;

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////
CDoorController::CDoorController()
{
	m_doorOpen = false;

	m_sunriseType = srsst_nautical;
	m_sunsetType = srsst_civil;

	m_relayms = 0;
}

CDoorController::~CDoorController()
{

}

void CDoorController::setup()
{
	// Setup door indicator
	pinMode(PIN_DOOR_STATE_LED, OUTPUT);

	// Setup the door relay
	pinMode(PIN_DOOR_RELAY, OUTPUT);
}

void CDoorController::saveSettings(CSaveController &_saveController)
{
	// Save settings
	_saveController.writeInt(getSunriseType());
	_saveController.writeInt(getSunsetType());
}

void CDoorController::loadSettings(CSaveController &_saveController)
{
	// Load settings
	eSunrise_Sunset_T sunriseType = (eSunrise_Sunset_T)_saveController.readInt();
	setSunriseType(sunriseType);

	eSunrise_Sunset_T sunsetType = (eSunrise_Sunset_T)_saveController.readInt();
	setSunsetType(sunsetType);

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController: Sunrise type is "));
	DEBUG_SERIAL.println((m_sunriseType == srsst_astronomical) ? PMS("astronomical.") :
						 (m_sunriseType == srsst_nautical) ? PMS("nautical.") :
						 (m_sunriseType == srsst_civil) ? PMS("civil.") :
						 (m_sunriseType == srsst_common) ? PMS("common.") :
						 PMS("** INVALID **")
						);

	DEBUG_SERIAL.print(PMS("CDoorController: Sunset type is "));
	DEBUG_SERIAL.println((m_sunsetType == srsst_astronomical) ? PMS("astronomical.") :
						 (m_sunsetType == srsst_nautical) ? PMS("nautical.") :
						 (m_sunsetType == srsst_civil) ? PMS("civil.") :
						 (m_sunsetType == srsst_common) ? PMS("common.") :
						 PMS("** INVALID **")
						);
#endif
}

void CDoorController::tick()
{
	if(m_relayms > 0 && m_relayms-- == 0)
		digitalWrite(PIN_DOOR_RELAY, LOW);
}

void CDoorController::checkTime()
{
	double current = g_sunCalc.getCurrentTime();
	double sunrise = g_sunCalc.getSunriseTime(m_sunriseType);
	double sunset = g_sunCalc.getSunsetTime(m_sunsetType);

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController: Door open from "));
	debugPrintDoubleTime(sunrise, false);
	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(sunset, false);
	DEBUG_SERIAL.println(PMS(" (UTC)"));
#endif

	// Update telemetry
	telemetrySend(telemetry_tag_doorOpenTime, sunrise);
	telemetrySend(telemetry_tag_doorCloseTime, sunset);

	if(!g_sunCalc.isValidTime(sunrise))
	{
		DEBUG_SERIAL.println(PMS("CDoorController: got invalid sunrise time"));
		return;
	}

	if(!g_sunCalc.isValidTime(sunset))
	{
		DEBUG_SERIAL.println(PMS("CDoorController: got invalid sunset time"));
		return;
	}

	bool doorShouldBeOpen = timeIsBetween(current, sunrise, sunset);
	if(m_doorOpen != doorShouldBeOpen)
	{
#ifdef COOPDOOR_CHANGE_BEEPER
		if(doorShouldBeOpen)
			g_beepController.beep(BEEP_FREQ_INFO, 900, 100, 2);
		else
			g_beepController.beep(BEEP_FREQ_INFO, 500, 500, 2);
#endif
		toggleDoorState();
	}

	// Send current door state to the telemetry port
	telemetrySend(telemetry_tag_door_state, (double)m_doorOpen);

#ifdef DEBUG_DOOR_CONTROLLER
	if(doorShouldBeOpen)
		DEBUG_SERIAL.println(PMS("CDoorController: Coop door should be OPEN."));
	else
		DEBUG_SERIAL.println(PMS("CDoorController: Coop door should be CLOSED."));
	DEBUG_SERIAL.println();
#endif
}

void CDoorController::toggleDoorState()
{
	m_doorOpen = !m_doorOpen;

	// Start the relay on timer
	m_relayms = CDoorController_RelayMS;
	digitalWrite(PIN_DOOR_RELAY, HIGH);

	// Show the door state on the LED (1 = open)
	digitalWrite(PIN_DOOR_STATE_LED, m_doorOpen);
}

