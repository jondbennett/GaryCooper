
#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>

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

	int year = gpsData.m_date.m_year;
	int month = gpsData.m_date.m_month;
	int day = gpsData.m_date.m_day;
	int hour = gpsData.m_time.m_hour;
	int minute = gpsData.m_time.m_minute;
	int second = gpsData.m_time.m_second;

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
