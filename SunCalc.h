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

// Various types of sunrise and sunset times
typedef enum
{
	srsst_invalid = -1,
	srsst_astronomical = 0,		// Longest day
	srsst_nautical,
	srsst_civil,
	srsst_common,				// Shortest day
} eSunrise_Sunset_T;

class CSunCalc
{
protected:

	double m_currentTime;

	double m_sunriseTime_astro;
	double m_sunsetTime_aastro;

	double m_sunriseTime_naut;
	double m_sunsetTime_naut;

	double m_sunriseTime_civil;
	double m_sunsetTime_civil;

	double m_sunriseTime_comm;
	double m_sunsetTime_comm;

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

	double getSunriseTime(eSunrise_Sunset_T _type);
	double getSunsetTime(eSunrise_Sunset_T _type);


	bool processGPSData(CGPSParserData &_gpsData);
};

bool timeIsBetween(double _currentTime, double _first, double _second);

#endif
