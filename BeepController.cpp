////////////////////////////////////////////////////
// State machine to control the beeper
////////////////////////////////////////////////////
#include <Arduino.h>

#include "BeepController.h"

// =================================================
// Setup my locals
// =================================================
CBeepController::CBeepController(int _iPinOut, int _iPinGnd)
{
	m_iFreq = 0;
	m_iOnTime = 0;
	m_iOffTime = 0;
	m_iRepeats = 0;

	m_bAlarm = false;

	m_iBeepOutPin = _iPinOut;
	m_iBeepGndPin = _iPinGnd;

	setState(Beep_Idle);
}

// =================================================
// Prepare to start beeping
// =================================================
void CBeepController::setup()
{
	// Set my output pin modes
	pinMode(m_iBeepOutPin, OUTPUT);
	digitalWrite(m_iBeepOutPin, LOW);

	// Set a ground for the speaker
	if(m_iBeepGndPin > 0)
	{
		pinMode(m_iBeepGndPin, OUTPUT);
		digitalWrite(m_iBeepGndPin, LOW);
	}

	// Stop the tone (if any)
	noTone(m_iBeepOutPin);

	// Terminate the alarm if any
	m_bAlarm = false;
}

void CBeepController::setState(Beep_StateT _eState)
{
	m_eState = _eState;
	tick();
}

// =================================================
// Tick beep cycle
// =================================================
void CBeepController::tick()
{
	switch(m_eState)
	{
	default:
	case Beep_Idle:
		break;

	case Beep_Start:
		tone(m_iBeepOutPin, m_iFreq, m_iOnTime);	// Start the tone
		m_ulBeginningTime = millis();
		setState(Beep_On);
		break;

	case Beep_On:
		/* Following line relies on overflow behavior of unsigned long,
		*  and requires that m_iOnTime not be negative. */
		if(millis() - m_ulBeginningTime >= m_iOnTime)	// Time expired?
		{
			/*^^^is noTone() below redundant with 3rd arg to tone() above? */
			noTone(m_iBeepOutPin);				// Stop the tone
			m_ulBeginningTime = millis();
			setState(Beep_Off);					// Go to off-time state
		}
		break;

	case Beep_Off:
		/* Following line relies on overflow behavior of unsigned long,
		*  and requires that m_iOffTime not be negative. */
		if(millis() - m_ulBeginningTime >= m_iOffTime)	// Off time expired?
		{
			if(!m_bAlarm && (m_iRepeats > 0))	// Decrement the repeat counter?
				--m_iRepeats;					// Not if there is an alarm

			if(m_iRepeats > 0)					// Repeat the on-off cycle?
				setState(Beep_Start);			// Start the cycle again
			else
				setState(Beep_Idle);			// All done repeating, so go to idle
		}
		break;
	}
}

// =================================================
// Initiate beep cycle
// =================================================
void CBeepController::beep(int _iFreq, int _iOnTime, int _iOffTime, int _iRepeats)
{
	// Hold alarm state regardless of other requests
	if(m_bAlarm) return;

	// Validate the value
	if(_iFreq < 32) return;
	if(_iOnTime < 0) return;
	if(_iOffTime < 0) return;
	if(_iRepeats < 1) return;

	// Clear the old tone
	noTone(m_iBeepOutPin);

	// Remember the values
	m_iFreq =  _iFreq;
	m_iOnTime = _iOnTime;
	m_iOffTime = _iOffTime;
	m_iRepeats = _iRepeats;

	// Start the beeping
	setState(Beep_Start);
}



