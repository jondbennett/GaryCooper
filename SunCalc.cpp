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
#include "MilliTimer.h"

#include "Pins.h"
#include "SunCalc.h"
#include "sunriset.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

extern CGPSParser g_GPSParser;

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
	m_sunriseTime = CSunCalc_INVALID_TIME;
	m_sunsetTime = CSunCalc_INVALID_TIME;
}

CSunCalc::~CSunCalc()
{

}

bool CSunCalc::processGPSData(CGPSParserData &_gpsData)
{
	// Assume he worst
	m_currentTime = CSunCalc_INVALID_TIME;
	m_sunriseTime = CSunCalc_INVALID_TIME;
	m_sunsetTime = CSunCalc_INVALID_TIME;

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.println();
#endif

	// Location
	double lat = _gpsData.m_position.m_lat;
	double lon = _gpsData.m_position.m_lon;

	// If we don't have a lock then
	// don't bother with the other stuff because
	// it will not be set
	if(!_gpsData.m_GPSLocked)
	{
#ifdef DEBUG_SUNCALC
		DEBUG_SERIAL.println(PMS("CSunCalc - GPS not locked."));
#endif
		reportError(telemetry_error_GPS_not_locked, true);
		return false;
	}
	else
	{
		reportError(telemetry_error_GPS_not_locked, false);
	}

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.println(PMS("CSunCalc - GPS locked."));
#endif

	// Date
	int year = _gpsData.m_date.m_year;
	int month = _gpsData.m_date.m_month;
	int day = _gpsData.m_date.m_day;
	int hour = _gpsData.m_time.m_hour;
	int minute = _gpsData.m_time.m_minute;

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.print(PMS("CSunCalc - Date: "));
	DEBUG_SERIAL.print(month);
	DEBUG_SERIAL.print(PMS("/"));
	DEBUG_SERIAL.print(day);
	DEBUG_SERIAL.print(PMS("/"));
	DEBUG_SERIAL.println(year);

	DEBUG_SERIAL.print(PMS("CSunCalc - Lat: "));
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
		reportError(telemetry_error_GPS_bad_data, true);
		return false;
	}
	else
	{
		reportError(telemetry_error_GPS_bad_data, false);
	}

	// Figure current time
	m_currentTime = hour + (minute / 60.);

	// Get the rise and set times
	civil_twilight( year, month, day, lon, lat,
					&m_sunriseTime, &m_sunsetTime);

	// Make sure the times make sense
	normalizeTime(m_sunriseTime);
	normalizeTime(m_sunsetTime);

#ifdef DEBUG_SUNCALC
	DEBUG_SERIAL.print(PMS("CSunCalc - Current Time (UTC): "));
	debugPrintDoubleTime(m_currentTime);

	DEBUG_SERIAL.print(PMS("CSunCalc - Sunrise - Sunset (UTC): "));
	debugPrintDoubleTime(m_sunriseTime, false);
	DEBUG_SERIAL.print(" - ");
	debugPrintDoubleTime(m_sunsetTime);

	DEBUG_SERIAL.println();
#endif

	return true;
}

void CSunCalc::sendTelemetry()
{
	const char *emptyS = PMS("");

	// Telemetry
	CGPSParserData gpsData = g_GPSParser.getGPSData();

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_GPSStatus);
	g_telemetry.sendTerm(gpsData.m_GPSLocked);
	if(gpsData.m_GPSLocked)
	{
		g_telemetry.sendTerm(gpsData.m_nSatellites);
		g_telemetry.sendTerm(gpsData.m_position.m_lat);
		g_telemetry.sendTerm(gpsData.m_position.m_lon);
	}
	else
	{
		g_telemetry.sendTerm(emptyS);
		g_telemetry.sendTerm(emptyS);
		g_telemetry.sendTerm(emptyS);
	}
	g_telemetry.transmissionEnd();

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_date_time);
	if(gpsData.m_GPSLocked)
	{
		g_telemetry.sendTerm(gpsData.m_date.m_year);
		g_telemetry.sendTerm(gpsData.m_date.m_month);
		g_telemetry.sendTerm(gpsData.m_date.m_day);
		g_telemetry.sendTerm(m_currentTime);
	}
	else
	{
		g_telemetry.sendTerm(emptyS);
		g_telemetry.sendTerm(emptyS);
		g_telemetry.sendTerm(emptyS);
		g_telemetry.sendTerm(emptyS);
	}
	g_telemetry.transmissionEnd();

	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_sun_times);
	g_telemetry.sendTerm(m_sunriseTime);
	g_telemetry.sendTerm(m_sunsetTime);
	g_telemetry.transmissionEnd();
}

bool timeIsBetween(double _currentTime, double _first, double _second)
{
	// See if they are practically the same
	double difference = fabs(_first - _second);
	if(difference < 0.02)
		return false;

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

// Deal with rolling to the next day
void normalizeTime(double &_t)
{
	while (_t < 0.) _t += 24.;
	while (_t > 24.) _t -= 24.;
}
