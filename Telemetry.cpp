
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>

#include "GaryCooper.h"

#include "Telemetry.h"

extern CGPSParser g_GPSParser;

void telemetrySetup()
{
	TELEMETRY_SERIAL_PORT.begin(TELEMETRY_BAUD_RATE);
}

void telemetrySend(telemetryTagE _tag, double _value)
{
	// Tag the telemetry
	CGPSParserData gpsData = g_GPSParser.getGPSData();

	int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
	if(gpsData.m_GPSLocked)
	{
		year = gpsData.m_date.m_year;
		month = gpsData.m_date.m_month;
		day = gpsData.m_date.m_day;
		hour = gpsData.m_time.m_hour;
		minute = gpsData.m_time.m_minute;
		second = gpsData.m_time.m_second;
	}

	const char *sep = ".";
	TELEMETRY_SERIAL_PORT.print(year);		TELEMETRY_SERIAL_PORT.print(sep);
	TELEMETRY_SERIAL_PORT.print(month);		TELEMETRY_SERIAL_PORT.print(sep);
	TELEMETRY_SERIAL_PORT.print(day);		TELEMETRY_SERIAL_PORT.print(sep);
	TELEMETRY_SERIAL_PORT.print(hour);		TELEMETRY_SERIAL_PORT.print(sep);
	TELEMETRY_SERIAL_PORT.print(minute);	TELEMETRY_SERIAL_PORT.print(sep);
	TELEMETRY_SERIAL_PORT.print(second);	TELEMETRY_SERIAL_PORT.print(PMS(":"));

	// Init the value which will include
	String tag(_tag);
	String value(_value);

	String telemetryString;
	telemetryString = tag;
	telemetryString += (",");
	telemetryString += (value);
	telemetryString += (",");

	unsigned sum = 0;
	for(unsigned _ = 0; _ < telemetryString.length(); ++_)
		sum ^= telemetryString[_];
	String sumString(sum);
	telemetryString += sumString;

	TELEMETRY_SERIAL_PORT.println(telemetryString);
}
