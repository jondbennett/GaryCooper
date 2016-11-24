////////////////////////////////////////////////////////////
// Coop Door Controller
////////////////////////////////////////////////////////////
#ifndef DoorController_h
#define DoorController_h

////////////////////////////////////////////////////////////
// Use GPS to decide when to open and close the coop door
////////////////////////////////////////////////////////////

// Abstract base class for all door motors. They are simple and stupid.
class IDoorMotor
{
public:

	virtual void setup() = 0;

	virtual telemetrycommandResponseT command(doorCommandE _command) = 0;
	virtual doorStateE getDoorState() = 0;

	virtual void tick() = 0;
};
extern IDoorMotor *getDoorMotor();

// The actual door controller. It is simple and smart.
class CDoorController
{
protected:
	doorStateE m_correctState;
	doorCommandE m_command;

	int  m_sunriseOffset;
	int  m_sunsetOffset;

	int m_stuckDoorS;
	CMilliTimer m_stuckDoorTimer;

public:
	CDoorController();
	virtual ~CDoorController();

	void setup();

	int getStuckDoorDelay()
	{
		return m_stuckDoorS;
	}

	telemetrycommandResponseT setStuckDoorDelay(int _delay)
	{
		if(_delay >= GARY_COOPER_MIN_DOOR_DELAY && _delay <= GARY_COOPER_MAX_DOOR_DELAY)
		{
			m_stuckDoorS = _delay;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}

	int getSunriseOffset()
	{
		return m_sunriseOffset;
	}

	telemetrycommandResponseT setSunriseOffset(int _sunriseOffset)
	{
		if(_sunriseOffset >= -GARY_COOPER_DOOR_MAX_TIME_OFFSET && _sunriseOffset <= GARY_COOPER_DOOR_MAX_TIME_OFFSET)
		{
			m_sunriseOffset = _sunriseOffset;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}

	int getSunsetOffset()
	{
		return m_sunsetOffset;
	}

	telemetrycommandResponseT setSunsetOffset(int _sunsetOffset)
	{
		if(_sunsetOffset >= -GARY_COOPER_DOOR_MAX_TIME_OFFSET && _sunsetOffset <= GARY_COOPER_DOOR_MAX_TIME_OFFSET)
		{
			m_sunsetOffset = _sunsetOffset;
			return telemetry_cmd_response_ack;
		}

		return telemetry_cmd_response_nak_invalid_value;
	}

	double getDoorOpenTime();
	double getDoorCloseTime();

	void saveSettings(CSaveController &_saveController, bool _defaults);
	void loadSettings(CSaveController &_saveController);

	void tick();

	void checkTime();
	void sendTelemetry();

	telemetrycommandResponseT command(doorCommandE _command);
};

#endif
