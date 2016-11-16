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

#include "Pins.h"
#include "SunCalc.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

#include "DoorMotor_GarageDoor.h"

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
	m_relayMS = 0;
	m_desiredDoorState = doorController_doorStateUnknown;
}

CDoorMotor_GarrageDoor::~CDoorMotor_GarrageDoor()
{

}

void CDoorMotor_GarrageDoor::setup()
{
	// Setup the door relay
	pinMode(PIN_DOOR_RELAY, OUTPUT);

	// Setup the door position switch sensor inputs
	pinMode(PIN_DOOR_OPEN_SWITCH, INPUT_PULLUP);
	pinMode(PINT_DOOR_CLOSED_SWITCH, INPUT_PULLUP);
}

void CDoorMotor_GarrageDoor::setDesiredDoorState(doorController_doorStateE _doorState)
{
#ifdef DEBUG_DOOR_MOTOR
	DEBUG_SERIAL.print(PMS("CDoorMotor_GarrageDoor - set desired state: "));
	DEBUG_SERIAL.println((_doorState == doorController_doorOpen) ? PMS("open.") :
						 (_doorState == doorController_doorClosed) ? PMS("closed.") :
						 (_doorState == doorController_doorStateUnknown) ? PMS("UNKNOWN.") :
						 PMS("*** INVALID ***"));
#endif

	if(_doorState != doorController_doorStateUnknown && getDoorState() != _doorState)
	{
		// Remember where we want to be
		m_desiredDoorState = _doorState;

		// Start the relay on timer
		m_relayMS = millis() + CDoorMotor_GarrageDoor_RelayMS;
		digitalWrite(PIN_DOOR_RELAY, HIGH);

#ifdef DEBUG_DOOR_MOTOR
		DEBUG_SERIAL.println(PMS("CDoorMotor_GarrageDoor - cycling relay."));
#endif

	}
}

doorController_doorStateE CDoorMotor_GarrageDoor::getDoorState()
{
	// NOTE, Inputs are active LOW, so a zero means that
	// the switch is closed and the door is in that position
	if(digitalRead(PIN_DOOR_OPEN_SWITCH) == 0)
		return doorController_doorOpen;

	if(digitalRead(PINT_DOOR_CLOSED_SWITCH) == 0)
		return doorController_doorClosed;

	return doorController_doorStateUnknown;
}

void CDoorMotor_GarrageDoor::tick()
{
	// Monitor the relay and turn it off
	// when the timer hits zero
	if(millis() > m_relayMS)
		digitalWrite(PIN_DOOR_RELAY, LOW);
}
