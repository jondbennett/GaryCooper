////////////////////////////////////////////////////////////
// Sun Calc
////////////////////////////////////////////////////////////
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>

#include "GaryCooper.h"
#include "Telemetry.h"
#include "SlidingBuf.h"
#include "Comm_Arduino.h"
#include "sunriset.h"
#include "SunCalc.h"

////////////////////////////////////////////////////////////
// Use GPS data to calculate sunrise, sunset, and current times.
// Times are in UTC and represented as float with hour in the
// integer and decimal hours instead of minutes in the fractional.
// ie 11:30 AM (UTC) is represented as 11.50, 11:45 AM (UTC)
// is represented as 11.75, and so on.
////////////////////////////////////////////////////////////

CSunCalc::CSunCalc()
{
	m_currentTime = CSunCalc_INVALID_TIME;

	m_sunriseTime_astro = CSunCalc_INVALID_TIME;
	m_sunsetTime_aastro = CSunCalc_INVALID_TIME;

	m_sunriseTime_naut = CSunCalc_INVALID_TIME;
	m_sunsetTime_naut = CSunCalc_INVALID_TIME;

	m_sunriseTime_civil = CSunCalc_INVALID_TIME;
	m_sunsetTime_civil = CSunCalc_INVALID_TIME;

	m_sunriseTime_comm = CSunCalc_INVALID_TIME;
	m_sunsetTime_comm = CSunCalc_INVALID_TIME;
}

CSunCalc::~CSunCalc()
{

}

double CSunCalc::getSunriseTime(eSunrise_Sunset_T _type)
{
	switch(_type)
	{
		case srsst_astronomical:
			return m_sunriseTime_astro;
		break;

		case srsst_nautical:
			return m_sunriseTime_naut;
		break;

		case srsst_civil:
			return m_sunriseTime_civil;
		break;

		case srsst_common:
			return m_sunriseTime_comm;
		break;

		default:
			break;
	}

	return CSunCalc_INVALID_TIME;
}

double CSunCalc::getSunsetTime(eSunrise_Sunset_T _type)
{
	switch(_type)
	{
		case srsst_astronomical:
			return m_sunsetTime_aastro;
		break;

		case srsst_nautical:
			return m_sunsetTime_naut;
		break;

		case srsst_civil:
			return m_sunsetTime_civil;
		break;

		case srsst_common:
			return m_sunsetTime_comm;
		break;

		default:
			break;
	}

	return CSunCalc_INVALID_TIME;
}

bool CSunCalc::processGPSData(CGPSParserData &_gpsData)
{
	// Assume he worst
	m_currentTime = CSunCalc_INVALID_TIME;

	m_sunriseTime_astro = CSunCalc_INVALID_TIME;
	m_sunsetTime_aastro = CSunCalc_INVALID_TIME;

	m_sunriseTime_naut = CSunCalc_INVALID_TIME;
	m_sunsetTime_naut = CSunCalc_INVALID_TIME;

	m_sunriseTime_civil = CSunCalc_INVALID_TIME;
	m_sunsetTime_civil = CSunCalc_INVALID_TIME;

	m_sunriseTime_comm = CSunCalc_INVALID_TIME;
	m_sunsetTime_comm = CSunCalc_INVALID_TIME;

	// If we don't have a lock then
	// don't bother with the other stuff because
	// it will not be set
#ifdef DEBUG_SUNCALC
		DEBUGSERIAL.println();
#endif

	if(!_gpsData.m_GPSLocked)
	{
#ifdef DEBUG_SUNCALC
		DEBUGSERIAL.println(PMS("CSunCalc: GPS not locked."));
#endif
					telemetrySend(telemetry_tag_error, telemetry_error_GPS_not_locked);
		return false;
	}

#ifdef DEBUG_SUNCALC
	DEBUGSERIAL.println(PMS("CSunCalc: GPS locked."));
#endif

	// Date
	int year = _gpsData.m_date.m_year;
	int month = _gpsData.m_date.m_month;
	int day = _gpsData.m_date.m_day;
	int hour = _gpsData.m_time.m_hour;
	int minute = _gpsData.m_time.m_minute;

	// Location
	double lat = _gpsData.m_position.m_lat;
	double lon = _gpsData.m_position.m_lon;

#ifdef DEBUG_SUNCALC
	DEBUGSERIAL.print(PMS("Date: "));
	DEBUGSERIAL.print(month);
	DEBUGSERIAL.print(PMS("/"));
	DEBUGSERIAL.print(day);
	DEBUGSERIAL.print(PMS("/"));
	DEBUGSERIAL.println(year);

	DEBUGSERIAL.print(PMS("Lat: "));
	DEBUGSERIAL.print(lat);
	DEBUGSERIAL.print(PMS(" Lon: "));
	DEBUGSERIAL.println(lon);
	DEBUGSERIAL.println();
#endif

	// Make sure we have good data
	if(!GPS_IS_VALID_DATA(year)) return false;
	if(!GPS_IS_VALID_DATA(month)) return false;
	if(!GPS_IS_VALID_DATA(day)) return false;
	if(!GPS_IS_VALID_DATA(hour)) return false;
	if(!GPS_IS_VALID_DATA(minute)) return false;
	if(!GPS_IS_VALID_DATA(lat)) return false;
	if(!GPS_IS_VALID_DATA(lon)) return false;


	// Figure current time
	m_currentTime = hour + (minute / 60.);

	// Get the rise and set times
	astronomical_twilight( year, month, day, lon, lat,
						   &m_sunriseTime_astro, &m_sunsetTime_aastro );

	nautical_twilight( year, month, day, lon, lat,
						   &m_sunriseTime_naut, &m_sunsetTime_naut );

	civil_twilight( year, month, day, lon, lat,
						   &m_sunriseTime_civil, &m_sunsetTime_civil );

	sun_rise_set( year, month, day, lon, lat,
						   &m_sunriseTime_comm, &m_sunsetTime_comm );

#ifdef DEBUG_SUNCALC
	DEBUGSERIAL.print(PMS("Current Time (UTC): ")); debugPrintDoubleTime(m_currentTime);

	DEBUGSERIAL.print(PMS("Astronomical (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_astro, false); DEBUGSERIAL.print(" - "); debugPrintDoubleTime(m_sunsetTime_aastro);


	DEBUGSERIAL.print(PMS("Nautical (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_naut, false); DEBUGSERIAL.print(" - "); debugPrintDoubleTime(m_sunsetTime_naut);

	DEBUGSERIAL.print(PMS("Civil (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_civil, false); DEBUGSERIAL.print(" - "); debugPrintDoubleTime(m_sunsetTime_civil);

	DEBUGSERIAL.print(PMS("Common (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_comm, false); DEBUGSERIAL.print(PMS(" - ")); debugPrintDoubleTime(m_sunsetTime_comm);
	DEBUGSERIAL.println();

#endif

	return true;
}
