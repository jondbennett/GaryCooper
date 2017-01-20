////////////////////////////////////////////////////////////
// A simple class to help with arduino based millisecond timers
////////////////////////////////////////////////////////////
#ifndef MilliTimer_h
#define MilliTimer_h


class CMilliTimer
{
public:
	typedef enum
	{
		notSet,
		running,
		expired,
	} CMilliTimerStateE;

protected:
	unsigned long m_interval;
	unsigned long m_startingMillis;

	CMilliTimerStateE m_state;

public:
	CMilliTimer()
	{
		reset();
	}

	virtual ~CMilliTimer() {}

	void start(unsigned long _time)
	{
		m_startingMillis = millis();
		m_interval = _time;

		m_state = running;
	}

	CMilliTimerStateE getState()
	{
		if((m_state == running) && ((millis() - m_startingMillis) > m_interval))
		{
			m_state = expired;
		}

		return m_state;
	}

	void reset()
	{
		m_interval = 0;
		m_startingMillis = 0;

		m_state = notSet;
	}
};

#endif
