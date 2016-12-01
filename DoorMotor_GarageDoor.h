////////////////////////////////////////////////////////////
// Door Motor - Open / close coop door by cycling a relay
////////////////////////////////////////////////////////////
#ifndef DoorMotor_GarageDoor_h
#define DoorMotor_GarageDoor_h

////////////////////////////////////////////////////////////
// Implementation of a chicken coop door controller that
// treats the door as a garage door style system with a
// button to open / close the door. This is broken out
// of the door controller so that other types of door
// motor (such as stepper motors) can be used with minimal
// change to the other software, especially the CDoorController class.
////////////////////////////////////////////////////////////
#define CDoorMotor_GarrageDoor_relayMS (500)				// Time to keep relay on for toggle
#define CDoorMotor_GarrageDoor_stuck_door_delayMS	(15000)	// Fifteen seconds should do it

class CDoorMotor_GarrageDoor : public IDoorMotor
{
protected:
	CMilliTimer m_relayTimer;			// Relay on timer
	CMilliTimer m_stuckDoorTimer;		// How long to wait for a door switch to close
	CMilliTimer m_lostSwitchesTimer;	// How long have the switches been gone?

	// This is kind of strange. Garage door controllers are
	// click to open, click to close. If I come up with none
	// of the position switches closed then I have no idea if
	// the door is open or closed. So, I click the relay once
	// in hopes that a switch will close at some point. This
	// is only done ONCE on my fist tick, and only if there
	// are no position sensor switches closed
	bool m_seekingKnownState;
	bool m_seekKnownStateRelayCommandIssued;

	doorStateE m_state;
	doorCommandE m_lastCommand;

	unsigned int getSwitches();
	bool uglySwitches();

public:
	CDoorMotor_GarrageDoor();
	virtual ~CDoorMotor_GarrageDoor();

	virtual void setup();

	virtual telemetrycommandResponseT command(doorCommandE _command);
	virtual doorStateE getDoorState();
	virtual void tick();
};

#endif
