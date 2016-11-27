////////////////////////////////////////////////////////////
// Door Motor - Open / close coop door by cycling a relay
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

#include "DoorMotor_GarageDoor.h"

typedef enum
{
	doorSwitchOpen 		= 1 << 0,
	doorSwitchClosed 	= 1 << 1,
} doorSwitchMaskE;

////////////////////////////////////////////////////////////
// Implementation of a chicken coop door controller that
// treats the door as a garage door style system with a
// button to open / close the door. This is broken out
// of the door controller so that other types of door
// motor (such as stepper motors) can be used with minimal
// change to the other software, especially the CDoorController class.
////////////////////////////////////////////////////////////
IDoorMotor *getDoorMotor()
{
	static CDoorMotor_GarrageDoor s_doorMotor;
	return &s_doorMotor;
}

CDoorMotor_GarrageDoor::CDoorMotor_GarrageDoor()
{
	m_seekingKnownState = true;
	m_state = doorState_unknown;
	m_lastCommand = (doorCommandE) - 1;
}

CDoorMotor_GarrageDoor::~CDoorMotor_GarrageDoor()
{

}

void CDoorMotor_GarrageDoor::setup()
{
	// Setup the door relay
	pinMode(PIN_DOOR_RELAY, OUTPUT);
	digitalWrite(PIN_DOOR_RELAY, RELAY_OFF);

	// Setup the door position switch sensor inputs
	pinMode(PIN_DOOR_OPEN_SWITCH, INPUT_PULLUP);
	pinMode(PIN_DOOR_CLOSED_SWITCH, INPUT_PULLUP);
}

telemetrycommandResponseT CDoorMotor_GarrageDoor::command(doorCommandE _command)
{
#ifdef DEBUG_DOOR_MOTOR
	DEBUG_SERIAL.print(PMS("CDoorMotor_GarrageDoor - command door: "));
	DEBUG_SERIAL.println((_command == doorCommand_open) ? PMS("open.") :
						 (_command == doorCommand_close) ? PMS("close.") :
						 PMS("*** INVALID ***"));
#endif

	if((_command != doorCommand_open) && (_command != doorCommand_close))
		return telemetry_cmd_response_nak_invalid_value;

	// Very special case at startup when coop door should be open
	if(m_seekingKnownState)
	{
#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - received command while seeking known state. Command delayed."));
#endif
		m_lastCommand = _command;
		return telemetry_cmd_response_ack;
	}

	// Only accept commands when I am in a stable state
	if((m_state != doorState_closed) && (m_state != doorState_open))
	{
#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - commmand() info:"));

#endif
		return telemetry_cmd_response_nak_not_ready;
	}

	// Well, act on the command
	if( ((m_state == doorState_closed) && (_command == doorCommand_open)) ||
			((m_state == doorState_open) && (_command == doorCommand_close)))
	{
#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - cycling relay."));
#endif

		// Remember my last valid command
		m_lastCommand = _command;

		// Start the relay on timer
		m_relayTimer.start(CDoorMotor_GarrageDoor_relayMS);
		digitalWrite(PIN_DOOR_RELAY, RELAY_ON);

		// Set door state to moving and start the stuck timer
		m_state = doorState_moving;
		m_stuckDoorTimer.start(CDoorMotor_GarrageDoor_stuck_door_delayMS);

		return telemetry_cmd_response_ack;
	}
	else
	{
#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - not commanded to opposite state, ignoring."));

#endif
		return telemetry_cmd_response_ack;
	}

	return telemetry_cmd_response_nak_internal_error;
}

doorStateE CDoorMotor_GarrageDoor::getDoorState()
{
	if(uglySwitches())
		return doorState_unknown;

	return m_state;
}

void CDoorMotor_GarrageDoor::tick()
{
	// Don't do anything if the switches are in an ugly state
	if(uglySwitches())
	{
		digitalWrite(PIN_DOOR_RELAY, RELAY_OFF);
		return;
	}

	// Monitor the relay and turn it off
	// when the timer hits zero
	// First "running" test returns to keep the door switch debounce
	// logic from messing with the relay timing
	if(m_relayTimer.getState() == CMilliTimerState_running)
		return;

	if(m_relayTimer.getState() == CMilliTimerState_expired)
	{
		m_relayTimer.reset();
		digitalWrite(PIN_DOOR_RELAY, RELAY_OFF);
	}

	///////////////////////////////////////////////////////////////////////
	// If I have not checked my initial state then do it now.
	if(m_seekingKnownState)
	{
		// I just started. This is my first tick. If I don't know
		// where the door is then toggle the relay to get it to
		// go somewhere!
		if((getSwitches() == 0) && (m_seekKnownStateRelayCommandIssued == false))
		{

#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - Started in unknown state, seeking a switch."));
#endif
			// Start the relay on timer
			m_relayTimer.start( CDoorMotor_GarrageDoor_relayMS);
			digitalWrite(PIN_DOOR_RELAY, RELAY_ON);

			// And a delay to see if we ever get there
			m_state = doorState_unknown;
			m_stuckDoorTimer.start(CDoorMotor_GarrageDoor_stuck_door_delayMS);

			m_seekKnownStateRelayCommandIssued = true;
		}

		// I'm waiting for a known state - good or bad.
		if(getSwitches() == doorSwitchOpen)
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - found open-switch, now in known state."));
#endif
			m_state = doorState_open;
			m_seekingKnownState = false;
			m_stuckDoorTimer.reset();
		}
		else if(getSwitches() == doorSwitchClosed)
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - found closed-switch, now in known state."));
#endif
			m_state = doorState_closed;
			m_seekingKnownState = false;
			m_stuckDoorTimer.reset();
		}
		else if(m_stuckDoorTimer.getState() == CMilliTimerState_expired)
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - time is up, entering unknown state."));
#endif
			m_state = doorState_unknown;
			m_seekingKnownState = false;
			m_stuckDoorTimer.reset();
		}

		return;
	}

	///////////////////////////////////////////////////////////////////////
	// We have passed the m_seekingKnownState startup phase. This is normal
	// operation
	switch(m_state)
	{
	case doorState_moving:
		if((m_lastCommand == doorCommand_open) && (getSwitches() == doorSwitchOpen))
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - reached command state: open"));
#endif
			m_state = doorState_open;
			m_stuckDoorTimer.reset();
		}
		else if((m_lastCommand == doorCommand_close) && (getSwitches() == doorSwitchClosed))
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - reached command state: closed"));
#endif
			m_state = doorState_closed;
			m_stuckDoorTimer.reset();
		}
		else if(m_stuckDoorTimer.getState() == CMilliTimerState_expired)
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - *** Timed out waiting for commanded state. ***"));
#endif
			m_state = doorState_unknown;
			m_stuckDoorTimer.reset();
		}
		return;

	case doorState_open:
		// Closed from the open state
		if((m_state == doorState_open) && (getSwitches() == doorSwitchClosed))
		{
	#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - spontaneous change to: closed"));
	#endif
			m_state = doorState_closed;
		}
		break;

	case doorState_closed:
		// Open from the closed state
		if((m_state == doorState_closed) && (getSwitches() == doorSwitchOpen))
		{
	#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - spontaneous change to: open"));
	#endif
			m_state = doorState_open;
		}
		break;

	case doorState_unknown:
		if(getSwitches() == doorSwitchOpen)
			m_state = doorState_open;

		if(getSwitches() == doorSwitchClosed)
			m_state = doorState_closed;
		break;

	default:
	#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - *** UNKNOWN STATE VALUE ***"));
	#endif
		return;
	}

	////////////////////////////////////////////
	// We are not moving, so we should be in the open or closed data.
	// Now we monitor for spontaneous changes in the door switches
	// Loss of door switches
	if(getSwitches() == 0)
	{
		// The switches went to 0. Someone might be out there
		// opening the door, so wait until we know for sure.
		if(m_lostSwitchesTimer.getState() == CMilliTimerState_notSet)
		{
			m_lostSwitchesTimer.start(CDoorMotor_GarrageDoor_stuck_door_delayMS);
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - both door switches open. Is the door moving?"));
#endif
		}
		else if(m_lostSwitchesTimer.getState() == CMilliTimerState_expired)
		{
#ifdef DEBUG_DOOR_MOTOR
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - spontaneous change to: * UNKNOWN *"));
#endif
			m_lostSwitchesTimer.reset();
			m_state = doorState_unknown;
		}
	}
	else
	{
#ifdef DEBUG_DOOR_MOTOR
		if(m_lostSwitchesTimer.getState() == CMilliTimerState_running)
			DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - switches recovered before timeout to unknown state."));
#endif
		m_lostSwitchesTimer.reset();
	}
}


static unsigned int rawSwitchRead()
{
	unsigned int mask = 0;

	// NOTE, Inputs are active LOW, so a zero means that
	// the switch is closed and the door is in that position
	if(digitalRead(PIN_DOOR_OPEN_SWITCH) == 0)
		mask |= doorSwitchOpen;

	if(digitalRead(PIN_DOOR_CLOSED_SWITCH) == 0)
		mask |= doorSwitchClosed;

	return mask;
}

unsigned int CDoorMotor_GarrageDoor::getSwitches()
{
	unsigned int mask1 = 0;
	unsigned int mask2 = 0;

	// Make sure the switches are not bouncing
	// and producing false readings.
	do
	{
		mask1 = rawSwitchRead();
		delay(2);

		mask2 = rawSwitchRead();
		delay(2);
	} while(mask1 != mask2);

	return mask2;
}

bool CDoorMotor_GarrageDoor::uglySwitches()
{
	if((getSwitches() & doorSwitchOpen) && (getSwitches() & doorSwitchClosed))
	{
#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - *** Ugly switches ***"));
#endif
		return true;
	}

	return false;
}

