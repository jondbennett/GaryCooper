////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#ifndef DoorController_h
#define DoorController_h

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////

#define CDoorController_Stuck_door_delayMS	(30000)	// Thirty seconds should do it

typedef enum
{
	doorController_doorStateUnknown = -1,
	doorController_doorClosed = 0,
	doorController_doorOpen,
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

	eSunrise_Sunset_T m_sunriseType;
	eSunrise_Sunset_T m_sunsetType;

	unsigned long m_stuckDoorMS;

public:
	CDoorController();
	virtual ~CDoorController();

	void setup();

	eSunrise_Sunset_T getSunriseType()
	{
		return m_sunriseType;
	}

	void setSunriseType(eSunrise_Sunset_T _sunriseType)
	{
		if(_sunriseType >= srsst_astronomical && _sunriseType <= srsst_common)
			m_sunriseType = _sunriseType;
	}

	eSunrise_Sunset_T getSunsetType()
	{
		return m_sunsetType;
	}

	void setSunsetType(eSunrise_Sunset_T _sunsetType)
	{
		if(_sunsetType >= srsst_astronomical && _sunsetType <= srsst_common)
			m_sunsetType = _sunsetType;
	}

	void saveSettings(CSaveController &_saveController, bool _defaults);
	void loadSettings(CSaveController &_saveController);

	void tick();
	void checkTime();
};

#endif
