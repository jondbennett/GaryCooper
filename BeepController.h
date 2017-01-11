////////////////////////////////////////////////////
// State machine to control the beeping
////////////////////////////////////////////////////
#ifndef BeepController_h
#define BeepController_h

////////////////////////////////////
// State enums
typedef enum
{
	Beep_Idle = 0,
	Beep_Start,
	Beep_On,
	Beep_Off,
} Beep_StateT;

class CBeepController
{
private:
	Beep_StateT m_eState;

	unsigned long m_ulBeginningTime; // Unit is 1 millisecond. (millis())

	void setState(Beep_StateT _eState);

	int m_iFreq;
	int m_iOnTime;
	int m_iOffTime;
	int m_iRepeats;

	bool m_bAlarm;

	int m_iBeepOutPin;
	int m_iBeepGndPin;

public:
	CBeepController(int _iPinOut, int _iPinGnd = -1);
	Beep_StateT state()
	{
		return m_eState;
	}

	void setup();
	void tick();

	void beep(int _iFreq, int _iOnTime, int _iOffTime, int _iRepeats);
};

#define BEEP_FREQ_BEST		(4000)		// Hz

#define BEEP_FREQ_ERROR		(4000)
#define BEEP_FREQ_INFO		(2000)

extern CBeepController g_oBeepController;
#endif

