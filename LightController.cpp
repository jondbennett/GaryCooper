////////////////////////////////////////////////////////////
// Chicken Coop light controller
////////////////////////////////////////////////////////////
#include <Arduino.h>

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

extern CBeepController g_beepController;
extern CComm_Arduino g_telemetryComm;
extern CSunCalc g_sunCalc;
extern CDoorController g_doorController;


// Deal with rolling to the next day
static void CLight_Controller_clipTime(double &_t)
{
	while (_t < 0.) _t += 24.;
	while (_t > 24.) _t -= 24.;
}

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
	m_minimumDayLength = CLight_Controller_Minimum_Day_Length;
	m_extraLightTime = CLight_Controller_Extra_Light_Time;
}

CLightController::~CLightController()
{
}

void CLightController::saveSettings(CSaveController &_saveController)
{
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
	DEBUGSERIAL.print(PMS("CLightController: Minimum day length is "));
	DEBUGSERIAL.println(getMinimumDayLength());

	DEBUGSERIAL.print(PMS("CLightController: Extra light time is "));
	DEBUGSERIAL.println(getExtraLightTime());
#endif
}

void CLightController::setup()
{

	// Setup the door relay
	pinMode(PIN_DOOR_RELAY, OUTPUT);
	digitalWrite(PIN_DOOR_RELAY, LOW);
}

void CLightController::checkTime()
{
	double currentTime = g_sunCalc.getCurrentTime();

	// Make sure the current time is valid
	if(CSunCalc_INVALID_TIME == currentTime) return;

	// Figure out when the door opens and closes
	double doorOpenTime = g_sunCalc.getSunriseTime(g_doorController.getSunriseType());
	double doorCloseTime = g_sunCalc.getSunsetTime(g_doorController.getSunsetType());

	// Make sure the door times are valid
	if(CSunCalc_INVALID_TIME == doorOpenTime) return;
	if(CSunCalc_INVALID_TIME == doorCloseTime) return;

	// Find mid day for the chickens
	double midDay = doorOpenTime + ((doorCloseTime - doorOpenTime) / 2.);
	CLight_Controller_clipTime(midDay);

	// Day length (for the chickens) is based on their normal wake / sleep cycle
	double dayLength = doorCloseTime - doorOpenTime;
	CLight_Controller_clipTime(midDay);

	// If the day length (eg in summer) is greater that the required illuminated day length
	// then we illuminate from civil sunset until the door closes
	bool supplementalIllumination = true;
	if(dayLength > m_minimumDayLength)
	{
#ifdef DEBUG_LIGHT_CONTROLLER
		DEBUGSERIAL.print(PMS("CLightController: this day is long enough, no supplemental light needed."));
#endif

		supplementalIllumination = false;
	}

	// Calculate light on and off times
	double halfIlluminationTime = m_minimumDayLength / 2.;

	double morningLightOnTime = (supplementalIllumination) ? midDay - halfIlluminationTime
															: doorOpenTime;
	double morningLightOffTime = doorOpenTime + m_extraLightTime;

	double eveningLightOnTime = doorCloseTime - m_extraLightTime;
	double eveningLightOffTime = (supplementalIllumination) ? midDay + halfIlluminationTime
															: doorCloseTime;

	// Make sure the adjusted times make sense
	CLight_Controller_clipTime(morningLightOnTime);
	CLight_Controller_clipTime(morningLightOffTime);

	CLight_Controller_clipTime(eveningLightOnTime);
	CLight_Controller_clipTime(eveningLightOffTime);

#ifdef DEBUG_LIGHT_CONTROLLER
	DEBUGSERIAL.print(PMS("CLightController: Chicken day length is "));
	debugPrintDoubleTime(dayLength);

	if(supplementalIllumination)
	{
		DEBUGSERIAL.print(PMS("CLightController: Supplemental lighting duration is "));
		debugPrintDoubleTime(m_minimumDayLength - dayLength);
	}
	else
	{
		DEBUGSERIAL.print(PMS("CLightController: No supplemental light needed"));
	}

	DEBUGSERIAL.print(PMS("CLightController: Chicken mid day (UTC) "));
	debugPrintDoubleTime(midDay);

	DEBUGSERIAL.print(PMS("CLightController: Morning light (UTC): "));
	debugPrintDoubleTime(morningLightOnTime, false);

	DEBUGSERIAL.print(PMS(" - "));
	debugPrintDoubleTime(morningLightOffTime);

	DEBUGSERIAL.print(PMS("CLightController: Evening light on (UTC): "));
	debugPrintDoubleTime(eveningLightOnTime, false);

	DEBUGSERIAL.print(PMS(" - "));
	debugPrintDoubleTime(eveningLightOffTime);

#endif

	// OK, now figure if the light should be on or off.
	if(currentTime <= midDay)
		setLightOn(timeIsBetween(currentTime, morningLightOnTime, morningLightOffTime));
	else
		setLightOn(timeIsBetween(currentTime, eveningLightOnTime, eveningLightOffTime));
}

void CLightController::setLightOn(bool _on)
{

#ifdef DEBUG_LIGHT_CONTROLLER
	DEBUGSERIAL.print(PMS("CLightController: Coop light should be "));
	DEBUGSERIAL.println((_on)? PMS("ON.") : PMS("OFF."));
#endif

	m_lightIsOn = _on;

	digitalWrite(PIN_DOOR_RELAY, m_lightIsOn?HIGH:LOW);

	telemetrySend(telemetry_tag_light_state, (double)m_lightIsOn);

}
