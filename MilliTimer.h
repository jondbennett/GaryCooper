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
	unsigned long m_time;
	CMilliTimerStateE m_state;

public:
	CMilliTimer()
	{
		m_time = 0L;
		m_state = notSet;
	}

	virtual ~CMilliTimer() {}

	void start(unsigned long _time)
	{
		m_time = millis() + _time;
		m_state = running;
	}

	CMilliTimerStateE getState()
	{
		if(m_state == running)
		{
			if(millis() > m_time)
				m_state = expired;
		}

		return m_state;
	}

	void reset()
	{
		m_time = 0L;
		m_state = notSet;
	}
};

#endif
