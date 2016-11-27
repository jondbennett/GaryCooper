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

class CLightController
{
protected:
	bool m_lightIsOn;			// Current on/off status of the light
	bool m_lastCorrectState;	// 'Correct' status on last check

	double m_minimumDayLength;

	double m_extraLightTimeMorning;
	double m_extraLightTimeEvening;

	double m_morningLightOnTime;
	double m_morningLightOffTime;

	double m_eveningLightOnTime;
	double m_eveningLightOffTime;

public:
	CLightController();
	virtual ~CLightController();

	void setup();

	double getMinimumDayLength()
	{
		return m_minimumDayLength;
	}

	telemetrycommandResponseT setMinimumDayLength(double _dayLen)
	{
		if(_dayLen >= GARY_COOPER_LIGHT_MIN_DAY_LENGTH && _dayLen <= GARY_COOPER_LIGHT_MAX_DAY_LENGTH)
		{
			m_minimumDayLength = _dayLen;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}

	double getExtraLightTimeMorning()
	{
		return m_extraLightTimeMorning;
	}


	double getExtraLightTimeEvening()
	{
		return m_extraLightTimeEvening;
	}

	telemetrycommandResponseT setExtraLightTimeMorning(double _elt)
	{
		if(_elt >= GARY_COOPER_LIGHT_MIN_EXTRA && _elt <= GARY_COOPER_LIGHT_MAX_EXTRA)
		{
			m_extraLightTimeMorning = _elt;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}


	telemetrycommandResponseT setExtraLightTimeEvening(double _elt)
	{
		if(_elt >= GARY_COOPER_LIGHT_MIN_EXTRA && _elt <= GARY_COOPER_LIGHT_MAX_EXTRA)
		{
			m_extraLightTimeEvening = _elt;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}

	void saveSettings(CSaveController &_saveController, bool _defaults);
	void loadSettings(CSaveController &_saveController);

	void checkTime();
	void sendTelemetry();

	telemetrycommandResponseT command(bool _on);
};

#endif
