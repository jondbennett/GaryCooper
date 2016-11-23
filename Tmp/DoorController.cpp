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

	m_sunriseOffset = 0.;
	m_sunsetOffset = 0.;

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
		setSunriseOffset(0.);
		setSunsetOffset(0.);
	}

	// Save settings
	_saveController.writeInt(getSunriseOffset());
	_saveController.writeInt(getSunsetOffset());
}

void CDoorController::loadSettings(CSaveController &_saveController)
{
	// Load settings
	int sunriseOffset = _saveController.readInt();
	setSunriseOffset(sunriseOffset);

	int sunsetOffset = _saveController.readInt();
	setSunsetOffset(sunsetOffset);

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController - sunrise offset is: "));
	DEBUG_SERIAL.println(getSunriseOffset());

	DEBUG_SERIAL.print(PMS("CDoorController - sunset offset is :"));
	DEBUG_SERIAL.println(getSunsetOffset());
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

	// We have commanded the door to move, so it should soon be
	// in the correct state
	if((m_stuckDoorMS > 0) && getDoorMotor()->getDoorState() == m_commandedState)
	{
		m_stuckDoorMS = 0;
#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - door motor has reached commanded state... Monitoring ends."));
#endif
		reportError(telemetry_error_door_not_responding, false);
		return;
	}

	// The door has not reached the correct state. Has the
	// timer timed out?
	if(m_stuckDoorMS && millis() > m_stuckDoorMS)
	{
#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - door motor has NOT reached commanded state... Monitoring continues."));
#endif
		m_stuckDoorMS = millis() + CDoorController_Stuck_door_delayMS;
		reportError(telemetry_error_door_not_responding, true);
	}
}

double CDoorController::getSunriseTime()
{
	double sunrise = g_sunCalc.getSunriseTime() + (getSunriseOffset() / 60.);
	normalizeTime(sunrise);
	return sunrise;
}

double CDoorController::getSunsetTime()
{
	double sunset = g_sunCalc.getSunsetTime() + (getSunsetOffset() / 60.);
	normalizeTime(sunset);
	return sunset;
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
		reportError(telemetry_error_no_door_motor, true);
		return;
	}
	else
	{
		reportError(telemetry_error_no_door_motor, false);
	}

	// If the door state is unknown and we are not waiting for it to
	// move then we have a problem
	if(m_stuckDoorMS == 0)
	{
		if(getDoorMotor()->getDoorState() == doorController_doorStateUnknown)
		{
#ifdef DEBUG_DOOR_CONTROLLER
			DEBUG_SERIAL.println(PMS("CDoorController - door motor in unknown state."));
#endif
			reportError(telemetry_error_door_motor_unknown_state, true);
			return;
		}
		else
		{
#ifdef DEBUG_DOOR_CONTROLLER
			DEBUG_SERIAL.println(PMS("CDoorController - door motor state OK."));
#endif
			reportError(telemetry_error_door_motor_unknown_state, false);
		}
	}

	// Get the times and keep going
	double current = g_sunCalc.getCurrentTime();
	double sunrise = getSunriseTime();
	double sunset = getSunsetTime();

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController - door open from: "));
	debugPrintDoubleTime(getSunriseTime(), false);
	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(getSunsetTime(), false);
	DEBUG_SERIAL.println(PMS(" (UTC)"));
#endif

	// Validate the values and report telemetry
	if(!g_sunCalc.isValidTime(sunrise))
	{
		reportError(telemetry_error_suncalc_invalid_time, true);
		return;
	}

	if(!g_sunCalc.isValidTime(sunset))
	{
		reportError(telemetry_error_suncalc_invalid_time, true);
		return;
	}
	reportError(telemetry_error_suncalc_invalid_time, false);

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

void CDoorController::sendTelemetry()
{
	double sunrise = getSunriseTime();
	double sunset = getSunsetTime();

	// Update telemetry starting with config info
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_door_config);
	g_telemetry.sendTerm((int)getSunriseOffset());
	g_telemetry.sendTerm((int)getSunsetOffset());
	g_telemetry.transmissionEnd();

	// Now, current times and door state
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_door_info);
	g_telemetry.sendTerm(sunrise);
	g_telemetry.sendTerm(sunset);

	// How we send the door motor state depends on if the door is
	// expected to be moving. We do this to mask the "door state unknown"
	// error message to the user.
	doorController_doorStateE doorMotorState = getDoorMotor()->getDoorState();
	if(m_stuckDoorMS == 0)
	{
		g_telemetry.sendTerm((int)doorMotorState);
	}
	else
	{
		if(doorMotorState == doorController_doorStateUnknown)
			g_telemetry.sendTerm((int)m_commandedState);
		else
			g_telemetry.sendTerm((int)doorMotorState);
	}

	g_telemetry.transmissionEnd();
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


