////////////////////////////////////////////////////////////
// Chicken Coop light controller
////////////////////////////////////////////////////////////
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "ICommInterface.h"
#include "Telemetry.h"
#include "TelemetryTags.h"
#include "MilliTimer.h"

#include "Pins.h"
#include "SunCalc.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

////////////////////////////////////////////////////////////
// Control the Chicken coop light to adjust for shorter days
// in the winter and keep egg production up.
//
// This object controls the coop light. It has the concept
// and setting of a minimum day length. If the current day
// length (how long the door is open) is shorter
// than the minimum day length then it provides supplemental
// illumination by turning the light on some time before opening
// the door in the morning, and turning it off some time after
// the door is closed in the evening.
//
// Separately, it leaves the light on for some time after opening
// the door in the morning to allow a little morning light to help
// the chickens get down from the perch, and turns the light
// some time before closing the door in the evening to help the
// chickens find their way to the coop and get on the perch.
////////////////////////////////////////////////////////////
CLightController::CLightController()
{
	m_lightIsOn = false;
	m_lastCorrectState = false;

	m_minimumDayLength = GARY_COOPER_LIGHT_DEF_DAY_LENGTH;
	m_extraLightTime = GARY_COOPER_LIGHT_DEF_EXTRA;

	m_morningLightOnTime = CSunCalc_INVALID_TIME;
	m_morningLightOffTime = CSunCalc_INVALID_TIME;

	m_eveningLightOnTime = CSunCalc_INVALID_TIME;
	m_eveningLightOffTime = CSunCalc_INVALID_TIME;
}

CLightController::~CLightController()
{
}

void CLightController::saveSettings(CSaveController &_saveController, bool _defaults)
{
	// Save defaults?
	if(_defaults)
	{
		setMinimumDayLength(GARY_COOPER_LIGHT_DEF_DAY_LENGTH);
		setExtraLightTime(GARY_COOPER_LIGHT_DEF_EXTRA);
	}

	// Save
	_saveController.writeDouble(getMinimumDayLength());
	_saveController.writeDouble(getExtraLightTime());
}

void CLightController::loadSettings(CSaveController &_saveController)
{
	// Load
	setMinimumDayLength(_saveController.readDouble());
	setExtraLightTime(_saveController.readDouble());

#ifdef DEBUG_LIGHT_CONTROLLER
	DEBUG_SERIAL.print(PMS("CLightController - minimum day length is: "));
	DEBUG_SERIAL.println(getMinimumDayLength());

	DEBUG_SERIAL.print(PMS("CLightController - extra light time is: "));
	DEBUG_SERIAL.println(getExtraLightTime());
#endif
}

void CLightController::setup()
{

	// Setup the light relay
	pinMode(PIN_LIGHT_RELAY, OUTPUT);
	digitalWrite(PIN_LIGHT_RELAY, RELAY_OFF);
}

void CLightController::checkTime()
{
	double currentTime = g_sunCalc.getCurrentTime();
	m_morningLightOnTime = CSunCalc_INVALID_TIME;
	m_morningLightOffTime = CSunCalc_INVALID_TIME;

	m_eveningLightOnTime = CSunCalc_INVALID_TIME;
	m_eveningLightOffTime = CSunCalc_INVALID_TIME;

	// Make sure the current time is valid
	if(CSunCalc_INVALID_TIME == currentTime) return;

	// Figure out when the door opens and closes
	double doorOpenTime = g_doorController.getDoorOpenTime();
	double doorCloseTime = g_doorController.getDoorCloseTime();

	// Make sure the door times are valid
	if((CSunCalc_INVALID_TIME == doorOpenTime) ||
			(CSunCalc_INVALID_TIME == doorCloseTime))
			return;

	// Find mid day for the chickens
	double midDay = doorOpenTime + ((doorCloseTime - doorOpenTime) / 2.);
	normalizeTime(midDay);

	// Day length (for the chickens) is based on their normal wake / sleep cycle
	double dayLength = doorCloseTime - doorOpenTime;
	normalizeTime(dayLength);

	// If the day length (eg in summer) is greater that the required illuminated day length
	// then we illuminate from civil sunset until the door closes
	bool supplementalIllumination = true;
	if(dayLength > m_minimumDayLength)
	{
#ifdef DEBUG_LIGHT_CONTROLLER
		DEBUG_SERIAL.println(PMS("CLightController - this day is long enough, no supplemental light needed."));
#endif
		supplementalIllumination = false;
	}

	// Calculate light on and off times
	double halfIlluminationTime = m_minimumDayLength / 2.;

	m_morningLightOnTime = (supplementalIllumination) ? midDay - halfIlluminationTime
						   : doorOpenTime;
	normalizeTime(m_morningLightOnTime);

	m_morningLightOffTime = doorOpenTime + m_extraLightTime;
	normalizeTime(m_morningLightOffTime);

	m_eveningLightOnTime = doorCloseTime - m_extraLightTime;
	normalizeTime(m_eveningLightOnTime);

	m_eveningLightOffTime = (supplementalIllumination) ? midDay + halfIlluminationTime
							: doorCloseTime;
	normalizeTime(m_eveningLightOffTime);

	// Check to see if the light status should change
	bool newCorrectState;
	if(currentTime <= midDay)
		newCorrectState = timeIsBetween(currentTime, m_morningLightOnTime, m_morningLightOffTime);
	else
		newCorrectState = timeIsBetween(currentTime, m_eveningLightOnTime, m_eveningLightOffTime);

	// If the light status should have changed since I last checked
	// then change the light's state
	if(newCorrectState != m_lastCorrectState)
		command(newCorrectState);
	m_lastCorrectState = newCorrectState;

#ifdef DEBUG_LIGHT_CONTROLLER
	DEBUG_SERIAL.print(PMS("CLightController - chicken day length is: "));
	debugPrintDoubleTime(dayLength);

	if(supplementalIllumination)
	{
		DEBUG_SERIAL.print(PMS("CLightController - supplemental lighting duration is: "));
		debugPrintDoubleTime(m_minimumDayLength - dayLength);
	}

	DEBUG_SERIAL.print(PMS("CLightController - chicken mid day (UTC): "));
	debugPrintDoubleTime(midDay);

	DEBUG_SERIAL.print(PMS("CLightController - morning light on (UTC): "));
	debugPrintDoubleTime(m_morningLightOnTime, false);

	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(m_morningLightOffTime);

	DEBUG_SERIAL.print(PMS("CLightController - evening light on (UTC): "));
	debugPrintDoubleTime(m_eveningLightOnTime, false);

	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(m_eveningLightOffTime);

	DEBUG_SERIAL.print(PMS("CLightController - light should be: "));
	DEBUG_SERIAL.println((m_lastCorrectState) ? PMS("ON.") : PMS("OFF."));

	DEBUG_SERIAL.print(PMS("CLightController - light is currently: "));
	DEBUG_SERIAL.println((m_lightIsOn) ? PMS("on.") : PMS("off."));
#endif
}

void CLightController::sendTelemetry()
{
	// Telemetry
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_light_config);
	g_telemetry.sendTerm(getMinimumDayLength());
	g_telemetry.sendTerm(getExtraLightTime());
	g_telemetry.transmissionEnd();

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_light_info);
	g_telemetry.sendTerm(m_morningLightOnTime);
	g_telemetry.sendTerm(m_morningLightOffTime);
	g_telemetry.sendTerm(m_eveningLightOnTime);
	g_telemetry.sendTerm(m_eveningLightOffTime);
	g_telemetry.sendTerm(m_lightIsOn);
	g_telemetry.transmissionEnd();
}

telemetrycommandResponseT CLightController::command(bool _on)
{

#ifdef DEBUG_LIGHT_CONTROLLER
	DEBUG_SERIAL.print(PMS("CLightController - setting coop light relay: "));
	DEBUG_SERIAL.println((_on) ? PMS("ON.") : PMS("OFF."));
#endif

	m_lightIsOn = _on;
	digitalWrite(PIN_LIGHT_RELAY, m_lightIsOn ? RELAY_ON : RELAY_OFF);

	return telemetry_cmd_response_ack;
}
