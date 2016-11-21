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
#define CDoorMotor_GarrageDoor_RelayMS (500)	// Time to keep relay on for toggle
class CDoorMotor_GarrageDoor : public IDoorMotor
{
protected:
	unsigned long m_relayMS;				// Relay on timer
	doorController_doorStateE m_desiredDoorState;

	// This is kind of strange. Garage door controllers are
	// click to open, click to close. If I come up with none
	// of the position switches closed the I have no idea where
	// the door is (open or closed), so I click the relay once
	// in hopes that a switch will close at some point. This
	// is only done ONCE on my fist tick, and only of there
	// are no position sensor switches closed
	bool m_seekSwitchCommanded;

public:
	CDoorMotor_GarrageDoor();
	virtual ~CDoorMotor_GarrageDoor();

	virtual void setup();

	virtual void setDesiredDoorState(doorController_doorStateE _doorState);
	virtual doorController_doorStateE getDoorState();

	virtual void tick();
};

#endif
