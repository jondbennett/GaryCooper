////////////////////////////////////////////////////
// State machine to control the beeping
////////////////////////////////////////////////////
#ifndef BeepController_h
#define BeepController_h

////////////////////////////////////
// State enums


class CBeepController
{
public:
	typedef enum
	{
		beepIdle = 0,
		beepStart,
		beepOn,
		beepOff,
	} beepStateE;

private:
	beepStateE m_state;

	unsigned long m_beginningTime; // Unit is 1 millisecond. (millis())

	void setState(beepStateE _state);

	int m_freq;
	unsigned long m_onTime;
	unsigned long m_offTime;
	int m_repeats;

	bool m_alarm;

	int m_beepOutPin;
	int m_beepGndPin;

public:
	CBeepController(int _pinOut, int _pinGnd = -1);
	beepStateE state()
	{
		return m_state;
	}

	void setup();
	void tick();

	void beep(int _freq, unsigned long _onTime, unsigned long _offTime, int _repeats);
};

#define BEEP_FREQ_BEST		(4000)		// Hz

#define BEEP_FREQ_ERROR		(4000)
#define BEEP_FREQ_INFO		(2000)

extern CBeepController g_beepController;
#endif

