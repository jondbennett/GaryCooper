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
#include "MilliTimer.h"

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
	m_correctState = doorState_unknown;
	m_command = (doorCommandE)-1;

	m_sunriseOffset = 0.;
	m_sunsetOffset = 0.;

	m_stuckDoorS = GARY_COOPER_DEF_DOOR_DELAY;
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
		setStuckDoorDelay(GARY_COOPER_DEF_DOOR_DELAY);
	}

	// Save settings
	_saveController.writeInt(getSunriseOffset());
	_saveController.writeInt(getSunsetOffset());
	_saveController.writeInt(getStuckDoorDelay());
}

void CDoorController::loadSettings(CSaveController &_saveController)
{
	// Load settings
	int sunriseOffset = _saveController.readInt();
	setSunriseOffset(sunriseOffset);

	int sunsetOffset = _saveController.readInt();
	setSunsetOffset(sunsetOffset);

	int stuckDoorDelay = _saveController.readInt();
	setStuckDoorDelay(stuckDoorDelay);

#ifdef DEBUG_DOOR_CONTROLLER
	DEBUG_SERIAL.print(PMS("CDoorController - sunrise offset is: "));
	DEBUG_SERIAL.println(getSunriseOffset());

	DEBUG_SERIAL.print(PMS("CDoorController - sunset offset is :"));
	DEBUG_SERIAL.println(getSunsetOffset());

	DEBUG_SERIAL.print(PMS("CDoorController - stuck door delay :"));
	DEBUG_SERIAL.println(getStuckDoorDelay());
	DEBUG_SERIAL.println();
#endif
}

void CDoorController::tick()
{
	// Tick the door motor
	if(getDoorMotor())
		getDoorMotor()->tick();

	// OK, there is a race condition here. If the door is commanded
	// to a different state, reaches that state, and then is
	// * spontaneously * returned to the original state before this
	// timer expires then a false failure will be reported.
	// I'm not interested in fixing it.

	// Check to see if the door is stuck
	if(m_stuckDoorTimer.getState() == CMilliTimerState_expired)
	{
		bool raiseAlarm = false;
		// Check for failure to open
		if((m_command == doorCommand_open) &&
			(getDoorMotor()->getDoorState() != doorState_open))
		{
			raiseAlarm = true;
		}

		// Check for failure to close
		if((m_command == doorCommand_close) &&
			(getDoorMotor()->getDoorState() != doorState_closed))
		{
			raiseAlarm = true;
		}

		// Alarm if error
		if(raiseAlarm)
		{
			reportError(telemetry_error_door_motor_unknown_not_responding, true);
		}
		else
		{
			m_stuckDoorTimer.reset();
			reportError(telemetry_error_door_motor_unknown_not_responding, false);
		}
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
	if(getDoorMotor()->getDoorState() == doorState_unknown)
	{

#ifdef DEBUG_DOOR_CONTROLLER
		DEBUG_SERIAL.println(PMS("CDoorController - door motor in unknown state."));
#endif
		reportError(telemetry_error_door_motor_unknown_state, true);
		return;
	}
	else
	{
		reportError(telemetry_error_door_motor_unknown_state, false);
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
	doorStateE newCorrectState =
		timeIsBetween(current, sunrise, sunset) ? doorState_open : doorState_closed;

	if(m_correctState != newCorrectState)
	{
#ifdef COOPDOOR_CHANGE_BEEPER
		if(newCorrectState)
			g_beepController.beep(BEEP_FREQ_INFO, 900, 100, 2);
		else
			g_beepController.beep(BEEP_FREQ_INFO, 500, 500, 2);
#endif
		m_correctState = newCorrectState;
		command((m_correctState == doorState_open) ? doorCommand_open : doorCommand_close);

#ifdef DEBUG_DOOR_CONTROLLER
		if(m_correctState == doorState_open)
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

	doorStateE doorState = getDoorMotor()->getDoorState();
	DEBUG_SERIAL.print(PMS("CDoorController - door motor reports: "));

	DEBUG_SERIAL.println((doorState == doorState_open) ? PMS("open.") :
						 (doorState == doorState_closed) ? PMS("closed.") :
						 (doorState == doorState_moving) ? PMS("moving") :
						 (doorState == doorState_unknown) ? PMS("UNKNOWN.") :
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
	g_telemetry.sendTerm((int)getStuckDoorDelay());
	g_telemetry.transmissionEnd();

	// Now, current times and door state
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_door_info);
	g_telemetry.sendTerm(sunrise);
	g_telemetry.sendTerm(sunset);
	g_telemetry.sendTerm((int)getDoorMotor()->getDoorState());
	g_telemetry.transmissionEnd();
}

telemetrycommandResponseT CDoorController::command(doorCommandE _command)
{
	// This had better work
	if(!getDoorMotor())
	{
#ifdef DEBUG_DOOR_CONTROLLER
			DEBUG_SERIAL.println(PMS("CDoorController - command - *** NO DOOR MOTOR FOUND ***"));
#endif

		return telemetry_cmd_response_nak_internal_error;
	}

	// Only accept valid commands
	if((_command != doorCommand_open) && (_command != doorCommand_close))
		return telemetry_cmd_response_nak_invalid_value;

	// Remember the command for checking door response
	m_command = _command;

	telemetrycommandResponseT response = getDoorMotor()->command(_command);

	if(response == telemetry_cmd_response_ack)
	{
		m_stuckDoorTimer.reset();
		unsigned long stuckDoorMS = (unsigned long)(m_stuckDoorS * MILLIS_PER_SECOND);
		m_stuckDoorTimer.start((unsigned long)stuckDoorMS);
	}

	return response;
}


