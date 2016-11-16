////////////////////////////////////////////////////////////
// Sun Calc
////////////////////////////////////////////////////////////
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>
#include <SaveController.h>

#include "ICommInterface.h"
#include "Telemetry.h"
#include "TelemetryTags.h"

#include "Pins.h"
#include "SunCalc.h"
#include "sunriset.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

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

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.println();
#endif

	// Location
	double lat = _gpsData.m_position.m_lat;
	double lon = _gpsData.m_position.m_lon;

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_GPSStatus);
	g_telemetry.sendTerm(_gpsData.m_GPSLocked);
	if(_gpsData.m_GPSLocked)
	{
		g_telemetry.sendTerm(_gpsData.m_nSatellites);
		g_telemetry.sendTerm(lat);
		g_telemetry.sendTerm(lon);
	}
	else
	{
		const char *empty = PMS("");
		g_telemetry.sendTerm(empty);
		g_telemetry.sendTerm(empty);
		g_telemetry.sendTerm(empty);
	}
	g_telemetry.transmissionEnd();

	// If we don't have a lock then
	// don't bother with the other stuff because
	// it will not be set
	if(!_gpsData.m_GPSLocked)
	{
#ifdef DEBUG_SUNCALC
		DEBUG_SERIAL.println(PMS("CSunCalc: GPS not locked."));
#endif
		reportError(telemetry_error_GPS_not_locked);
		return false;
	}

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.println(PMS("CSunCalc: GPS locked."));
#endif

	// Date
	int year = _gpsData.m_date.m_year;
	int month = _gpsData.m_date.m_month;
	int day = _gpsData.m_date.m_day;
	int hour = _gpsData.m_time.m_hour;
	int minute = _gpsData.m_time.m_minute;



#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.print(PMS("Date: "));
	DEBUG_SERIAL.print(month);
	DEBUG_SERIAL.print(PMS("/"));
	DEBUG_SERIAL.print(day);
	DEBUG_SERIAL.print(PMS("/"));
	DEBUG_SERIAL.println(year);

	DEBUG_SERIAL.print(PMS("Lat: "));
	DEBUG_SERIAL.print(lat);
	DEBUG_SERIAL.print(PMS(" Lon: "));
	DEBUG_SERIAL.println(lon);
	DEBUG_SERIAL.println();
#endif

	// Make sure we have good data
	if(!GPS_IS_VALID_DATA(year) ||
			!GPS_IS_VALID_DATA(month) ||
			!GPS_IS_VALID_DATA(day) ||
			!GPS_IS_VALID_DATA(hour) ||
			!GPS_IS_VALID_DATA(minute) ||
			!GPS_IS_VALID_DATA(lat) ||
			!GPS_IS_VALID_DATA(lon))
	{
		reportError(telemetry_error_GPS_bad_data);
		return false;
	}

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

	// Telemetry

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_date_time);
	g_telemetry.sendTerm(year);
	g_telemetry.sendTerm(month);
	g_telemetry.sendTerm(day);
	g_telemetry.sendTerm(m_currentTime);
	g_telemetry.transmissionEnd();

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.print(PMS("Current Time (UTC): "));
	debugPrintDoubleTime(m_currentTime);

	DEBUG_SERIAL.print(PMS("Astronomical (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_astro, false);
	DEBUG_SERIAL.print(" - ");
	debugPrintDoubleTime(m_sunsetTime_aastro);


	DEBUG_SERIAL.print(PMS("Nautical (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_naut, false);
	DEBUG_SERIAL.print(" - ");
	debugPrintDoubleTime(m_sunsetTime_naut);

	DEBUG_SERIAL.print(PMS("Civil (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_civil, false);
	DEBUG_SERIAL.print(" - ");
	debugPrintDoubleTime(m_sunsetTime_civil);

	DEBUG_SERIAL.print(PMS("Common (UTC): "));
	debugPrintDoubleTime(m_sunriseTime_comm, false);
	DEBUG_SERIAL.print(PMS(" - "));
	debugPrintDoubleTime(m_sunsetTime_comm);
	DEBUG_SERIAL.println();
#endif

	return true;
}

bool timeIsBetween(double _currentTime, double _first, double _second)
{
	if(_first < _second)
	{
		if((_currentTime >= _first) && (_currentTime < _second))
			return true;
		else
			return false;
	}
	else
	{
		if(	((_currentTime >= _first) && (_currentTime < 24.)) ||
				((_currentTime > 0.) && (_currentTime < _second)) )
			return true;
		else
			return false;
	}
}
