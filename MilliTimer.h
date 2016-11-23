////////////////////////////////////////////////////////////
// A simple class to help with arduino based millisecond timers
////////////////////////////////////////////////////////////
#ifndef MilliTimer_h
#define MilliTimer_h

typedef enum
{
	CMilliTimerState_notSet,
	CMilliTimerState_running,
	CMilliTimerState_expired,
} CMilliTimerStateE;

class CMilliTimer
{
protected:
	unsigned long m_time;
	CMilliTimerStateE m_state;

public:
	CMilliTimer()
	{
		m_time = 0L;
		m_state = CMilliTimerState_notSet;
	}

	virtual ~CMilliTimer() {}

	void start(unsigned long _time)
	{
		m_time = millis() + _time;
		m_state = CMilliTimerState_running;
	}

	CMilliTimerStateE getState()
	{
		if(m_state == CMilliTimerState_running)
		{
			if(millis() > m_time)
				m_state = CMilliTimerState_expired;
		}

		return m_state;
	}

	void reset()
	{
		m_time = 0L;
		m_state = CMilliTimerState_notSet;
	}
};

#endif
