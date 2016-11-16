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
#define CDoorMotor_GarrageDoor_RelayMS (250)	// Time to keep relay on for toggle
class CDoorMotor_GarrageDoor : public IDoorMotor
{
protected:
	unsigned long m_relayMS;				// Relay on timer
	doorController_doorStateE m_desiredDoorState;

public:
	CDoorMotor_GarrageDoor();
	virtual ~CDoorMotor_GarrageDoor();

	virtual void setup();

	virtual void setDesiredDoorState(doorController_doorStateE _doorState);
	virtual doorController_doorStateE getDoorState();

	virtual void tick();
};

#endif
