////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#ifndef DoorController_h
#define DoorController_h

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////
#define CDoorController_RelayMS (250)	// Time to keep relay on for toggle

class CDoorController
{
protected:
	bool m_doorOpen;

	eSunrise_Sunset_T m_sunriseType;
	eSunrise_Sunset_T m_sunsetType;

	unsigned long m_relayms;	// How long to keep the relay on

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
		m_sunriseType = _sunriseType;
	}

	eSunrise_Sunset_T getSunsetType()
	{
		return m_sunsetType;
	}
	void setSunsetType(eSunrise_Sunset_T _sunsetType)
	{
		m_sunsetType = _sunsetType;
	}

	void saveSettings(CSaveController &_saveController);
	void loadSettings(CSaveController &_saveController);

	void tick();
	void checkTime();

	void toggleDoorState();
};

#endif
