////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "ICommInterface.h"
#include "TelemetryTags.h"
#include "Telemetry.h"

#include "Pins.h"
#include "SunCalc.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////
CDoorController::CDoorController()
{
	m_correctState = doorController_doorStateUnknown;
	m_commandedState = doorController_doorStateUnknown;

	m_sunriseType = srsst_nautical;
	m_sunsetType = srsst_civil;

	m_stuckDoorMS = 0;
}

CDoorController::~CDoorController()
{
}

void CDoorController::setup()
{
	if(getDoorMotor())
		getDoorMotor()->setup();
}

void CDoorController::saveSettings(CSaveController &_saveController, bool _defaults)
{
	// Should I setup for default settings?
	if(_defaults)
	{
		setSunriseType(srsst_nautical);
		setSunsetType(srsst_civil);
	}

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
	DEBUG_SERIAL.print(PMS("CDoorController - sunrise type is: "));
	DEBUG_SERIAL.println((m_sunriseType == srsst_astronomical) ? PMS("astronomical.") :
						 (m_sunriseType == srsst_nautical) ? PMS("nautical.") :
						 (m_sunriseType == srsst_civil) ? PMS("civil.") :
						 (m_sunriseType == srsst_common) ? PMS("common.") :
						 PMS("** INVALID **")
						);

	DEBUG_SERIAL.print(PMS("CDoorController - sunset type is :"));
	DEBUG_SERIAL.println((m_sunsetType == srsst_astronomical) ? PMS("astronomical.") :
						 (m_sunsetType == srsst_nautical) ? PMS("nautical.") :
						 (m_sunsetType == srsst_civil) ? PMS("civil.") :
						 (m_sunsetType == srsst_common) ? PMS("common.") :
						 PMS("** INVALID **")
						);
	DEBUG_SERIAL.println();
#endif
}

void CDoorController::tick()
{
	// Let the door motor do its thing
	if(!getDoorMotor())
		return;

	getDoorMotor()->tick();

	// See of the door has been commanded to a specific state.
	// If not then there is nothing to do
	if(m_commandedState == doorController_doorStateUnknown)
		return;

	// We have commanded the door to move, so it should now be
	// in the correct state
	if(m_stuckDoorMS && millis() > m_stuckDoorMS)
	{
		// We've waited long enough, the door should
		// be in the correct state by now
		if(getDoorMotor()->getDoorState() != m_commandedState)
		{
			m_stuckDoorMS = millis() + CDoorController_Stuck_door_delayMS;
			reportError(telemetry_error_door_not_responding);
		}
		else
		{
			m_stuckDoorMS = 0;
		}
	}
}

void CDoorController::checkTime()
{
	// First of all, if the door motor does not know the door state
	// then there is nothing to do.
	if(!getDoorMotor())
	{
#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - no door motor found."));
		DEBUG_SERIAL.println();
#endif
		reportError(telemetry_error_no_door_motor);
		return;
	}

	if(getDoorMotor()->getDoorState() == doorController_doorStateUnknown)
	{
#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - door motor in unknown state."));
#endif
		reportError(telemetry_error_door_motor_unknown_state);
		return;
	}

	double current = g_sunCalc.getCurrentTime();
	double sunrise = g_sunCalc.getSunriseTime(m_sunriseType);
	double sunset = g_sunCalc.getSunsetTime(m_sunsetType);

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController - door open from: "));
	debugPrintDoubleTime(sunrise, false);
	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(sunset, false);
	DEBUG_SERIAL.println(PMS(" (UTC)"));
#endif

	// Update telemetry starting with config info
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_door_config);
	g_telemetry.sendTerm((int)getSunriseType());
	g_telemetry.sendTerm((int)getSunsetType());
	g_telemetry.transmissionEnd();

	// Now, current times and door state
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_door_info);
	g_telemetry.sendTerm(sunrise);
	g_telemetry.sendTerm(sunset);
	g_telemetry.sendTerm((int)getDoorMotor()->getDoorState());
	g_telemetry.transmissionEnd();

	if(!g_sunCalc.isValidTime(sunrise))
	{
		reportError(telemetry_error_suncalc_invalid_time);
		return;
	}

	if(!g_sunCalc.isValidTime(sunset))
	{
		reportError(telemetry_error_suncalc_invalid_time);
		return;
	}

	// Check to see if the door state should change.
	// NOTE: we do it this way because a blind setting of the state
	// each time would make it impossible to remotely command the door
	// because the DoorController would keep resetting it to the "correct"
	// state each minute. So, we check for changes in the correct state and
	// then tell the door motor where we want it. That way, if you close the door
	// early, perhaps the birds have already cooped up, then it won't keep forcing
	// the door back to open until sunset.
	doorController_doorStateE newCorrectState =
		timeIsBetween(current, sunrise, sunset) ? doorController_doorOpen : doorController_doorClosed;

	if(m_correctState != newCorrectState)
	{
#ifdef COOPDOOR_CHANGE_BEEPER
		if(newCorrectState)
			g_beepController.beep(BEEP_FREQ_INFO, 900, 100, 2);
		else
			g_beepController.beep(BEEP_FREQ_INFO, 500, 500, 2);
#endif
		m_correctState = newCorrectState;
		setDoorState(m_correctState);

#ifdef DEBUG_DOOR_CONTROLLER
		if(m_correctState)
			DEBUG_SERIAL.println(PMS("CDoorController - opening coop door."));
		else
			DEBUG_SERIAL.println(PMS("CDoorController - closing coop door."));
#endif
	}

#ifdef DEBUG_DOOR_CONTROLLER
	if(m_correctState)
		DEBUG_SERIAL.println(PMS("CDoorController - coop door should be OPEN."));
	else
		DEBUG_SERIAL.println(PMS("CDoorController - coop door should be CLOSED."));

	doorController_doorStateE doorState = getDoorMotor()->getDoorState();
	DEBUG_SERIAL.print(PMS("CDoorController - door motor reports: "));

	DEBUG_SERIAL.println((doorState == doorController_doorOpen) ? PMS("open.") :
						 (doorState == doorController_doorClosed) ? PMS("closed.") :
						 (doorState == doorController_doorStateUnknown) ? PMS("UNKNOWN.") :
						 PMS("*** INVALID ***"));
	DEBUG_SERIAL.println();
#endif
}

void CDoorController::setDoorState(doorController_doorStateE _state)
{
	// Remember the commanded state, no matter who commanded it
	m_commandedState = _state;
	if(getDoorMotor()->getDoorState() != m_commandedState)
	{
		getDoorMotor()->setDesiredDoorState(m_commandedState);

#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - starting door response monitor."));
#endif
		m_stuckDoorMS = millis() + CDoorController_Stuck_door_delayMS;
	}

}


