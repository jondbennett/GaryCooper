////////////////////////////////////////////////////////////
// Sun Calc
////////////////////////////////////////////////////////////
#ifndef SunCalc_h
#define SunCalc_h

////////////////////////////////////////////////////////////
// Use GPS data to calculate sunrise, sunset, and current times.
// Times are in UTC and represented as float with hour in the
// integer and decimal hours instead of minutes in the fractional.
// ie 11:30 AM (UTC) is represented as 11.50, 11:45 AM (UTC)
// is represented as 11.75, and so on.
////////////////////////////////////////////////////////////
#define CSunCalc_INVALID_TIME	(-999)
class CSunCalc
{
protected:

	double m_currentTime;
	double m_sunriseTime;	// Civil
	double m_sunsetTime;	// Civil

public:
	CSunCalc();
	virtual ~CSunCalc();

	bool isValidTime(float _t)
	{
		if((_t >= 0) && (_t < 24.))
			return true;
		return false;
	}

	double getCurrentTime()
	{
		return m_currentTime;
	}


	double getSunriseTime()
	{
		return m_sunriseTime;
	}

	double getSunsetTime()
	{
		return m_sunsetTime;
	}

	bool processGPSData(CGPSParserData &_gpsData);
	void sendTelemetry();
};

// Deal with rolling to the next day
void normalizeTime(double &_t);

// Check time ranges
bool timeIsBetween(double _currentTime, double _first, double _second);

#endif
