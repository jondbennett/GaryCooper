////////////////////////////////////////////////////////////
// Light Controller
////////////////////////////////////////////////////////////
#ifndef LightController_h
#define LightController_h

////////////////////////////////////////////////////////////
// Control the Chicken coop light to adjust for shorter days
// in the winter and keep egg production up.
//
// This object controls the coop light. It has the concept
// and setting of a minimum day length. If the current day
// length (how long the door is open) is shorter
// than the minimum day length then it provides supplemental
// illumination by turning the light on some time before opening
// the door in the morning, and turning it off some time after
// the door is closed in the evening.
//
// Separately, it leaves the light on for some time after opening
// the door in the morning to allow a little morning light to help
// the chickens get down from the perch, and turns the light
// some time before closing the door in the evening to help the
// chickens find their way to the coop and get on the perch.
////////////////////////////////////////////////////////////
#define CLight_Controller_Minimum_Day_Length	(13.)	// Minimum day length
#define CLight_Controller_Extra_Light_Time		(0.5)	// Early light on or off duration

class CLightController
{
protected:
	bool m_lightIsOn;

	double m_minimumDayLength;
	double m_extraLightTime;

public:
	CLightController();
	virtual ~CLightController();

	void setup();

	double getMinimumDayLength()
	{
		return m_minimumDayLength;
	}
	void setMinimumDayLength(double _dayLen)
	{
		m_minimumDayLength = _dayLen;
	}

	double getExtraLightTime()
	{
		return m_extraLightTime;
	}
	void setExtraLightTime(double _elt)
	{
		m_extraLightTime = _elt;
	}


	void saveSettings(CSaveController &_saveController);
	void loadSettings(CSaveController &_saveController);

	void checkTime();

	void setLightOn(bool _on);
};

#endif
