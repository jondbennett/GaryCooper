////////////////////////////////////////////////////
// State machine to control the beeper
////////////////////////////////////////////////////
#include <Arduino.h>

#include "BeepController.h"

// =================================================
// Setup my locals
// =================================================
CBeepController::CBeepController(int _pinOut, int _pinGnd)
{
	m_freq = 0;
	m_onTime = 0;
	m_offTime = 0;
	m_repeats = 0;

	m_alarm = false;

	m_beepOutPin = _pinOut;
	m_beepGndPin = _pinGnd;

	setState(beepIdle);
}

// =================================================
// Prepare to start beeping
// =================================================
void CBeepController::setup()
{
	// Set my output pin modes
	pinMode(m_beepOutPin, OUTPUT);
	digitalWrite(m_beepOutPin, LOW);

	// Set a ground for the speaker
	if(m_beepGndPin > 0)
	{
		pinMode(m_beepGndPin, OUTPUT);
		digitalWrite(m_beepGndPin, LOW);
	}

	// Stop the tone (if any)
	noTone(m_beepOutPin);

	// Terminate the alarm if any
	m_alarm = false;
}

void CBeepController::setState(beepStateE _state)
{
	m_state = _state;
	tick();
}

// =================================================
// Tick beep cycle
// =================================================
void CBeepController::tick()
{
	switch(m_state)
	{
	default:
	case beepIdle:
		break;

	case beepStart:
		tone(m_beepOutPin, m_freq, m_onTime);	// Start the tone
		m_beginningTime = millis();
		setState(beepOn);
		break;

	case beepOn:
		/* Following line relies on overflow behavior of unsigned long,
		*  and requires that m_onTime not be negative. */
		if(millis() - m_beginningTime >= m_onTime)	// Time expired?
		{
			/*^^^is noTone() below redundant with 3rd arg to tone() above? */
			noTone(m_beepOutPin);				// Stop the tone
			m_beginningTime = millis();
			setState(beepOff);					// Go to off-time state
		}
		break;

	case beepOff:
		/* Following line relies on overflow behavior of unsigned long,
		*  and requires that m_offTime not be negative. */
		if(millis() - m_beginningTime >= m_offTime)	// Off time expired?
		{
			if(!m_alarm && (m_repeats > 0))	// Decrement the repeat counter?
				--m_repeats;					// Not if there is an alarm

			if(m_repeats > 0)					// Repeat the on-off cycle?
				setState(beepStart);			// Start the cycle again
			else
				setState(beepIdle);			// All done repeating, so go to idle
		}
		break;
	}
}

// =================================================
// Initiate beep cycle
// =================================================
void CBeepController::beep(int _freq, unsigned long _onTime, unsigned long _offTime, int _repeats)
{
	// Hold alarm state regardless of other requests
	if(m_alarm) return;

	// Validate the value
	if(_freq < 32) return;
	if(_repeats < 1) return;

	// Clear the old tone
	noTone(m_beepOutPin);

	// Remember the values
	m_freq =  _freq;
	m_onTime = _onTime;
	m_offTime = _offTime;
	m_repeats = _repeats;

	// Start the beeping
	setState(beepStart);
}



