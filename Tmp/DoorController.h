////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#ifndef DoorController_h
#define DoorController_h

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////

#define CDoorController_Stuck_door_delayMS	(15000)	// Fifteen seconds should do it
typedef enum
{
	doorController_doorStateUnknown = -1,
	doorController_doorClosed = 0,
	doorController_doorOpen,
	doorController_moving,
} doorController_doorStateE;

class IDoorMotor
{
public:

	virtual void setup() = 0;

	virtual void setDesiredDoorState(doorController_doorStateE _doorState) = 0;
	virtual doorController_doorStateE getDoorState() = 0;

	virtual void tick() = 0;
};
extern IDoorMotor *getDoorMotor();

class CDoorController
{
protected:
	doorController_doorStateE m_correctState;
	doorController_doorStateE m_commandedState;

	int  m_sunriseOffset;
	int  m_sunsetOffset;

	unsigned long m_stuckDoorMS;

public:
	CDoorController();
	virtual ~CDoorController();

	void setup();

	int getSunriseOffset()
	{
		return m_sunriseOffset;
	}

	bool setSunriseOffset(int _sunriseOffset)
	{
		if(_sunriseOffset >= -GARY_COOPER_DOOR_MAX_TIME_OFFSET && _sunriseOffset <= GARY_COOPER_DOOR_MAX_TIME_OFFSET)
		{
			m_sunriseOffset = _sunriseOffset;
			return true;
		}

		return false;
	}

	int getSunsetOffset()
	{
		return m_sunsetOffset;
	}

	bool setSunsetOffset(int _sunsetOffset)
	{
		if(_sunsetOffset >= -GARY_COOPER_DOOR_MAX_TIME_OFFSET && _sunsetOffset <= GARY_COOPER_DOOR_MAX_TIME_OFFSET)
		{
			m_sunsetOffset = _sunsetOffset;
			return true;
		}

		return false;
	}

	double getSunriseTime();
	double getSunsetTime();

	void saveSettings(CSaveController &_saveController, bool _defaults);
	void loadSettings(CSaveController &_saveController);

	void tick();

	void checkTime();
	void sendTelemetry();

	void setDoorState(doorController_doorStateE _state);
};

#endif
