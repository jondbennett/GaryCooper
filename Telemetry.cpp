#include <Arduino.h>

#include <PMS.h>
#include <GPSParser.h>

#include "Pins.h"
#include "GaryCooper.h"

#include "Telemetry.h"

extern CGPSParser g_GPSParser;

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

	// Info for the separators
	const char *start = "$";
	const char *data_sep = ",";
	const char *cs_sep = "*";

	// Dump the timestamp
	TELEMETRY_SERIAL.print(start);
	TELEMETRY_SERIAL.print(year);
	TELEMETRY_SERIAL.print(data_sep);
	TELEMETRY_SERIAL.print(month);
	TELEMETRY_SERIAL.print(data_sep);
	TELEMETRY_SERIAL.print(day);
	TELEMETRY_SERIAL.print(data_sep);
	TELEMETRY_SERIAL.print(hour);
	TELEMETRY_SERIAL.print(data_sep);
	TELEMETRY_SERIAL.print(minute);
	TELEMETRY_SERIAL.print(data_sep);
	TELEMETRY_SERIAL.print(second);
	TELEMETRY_SERIAL.print(data_sep);

	// Dump the actual data
	String tag(_tag);
	String value(_value);

	String telemetryString;
	telemetryString = tag;
	telemetryString += data_sep;
	telemetryString += (value);

	// Calculate the checksum
	unsigned sum = 0;
	for(unsigned _ = 0; _ < telemetryString.length(); ++_)
		sum ^= telemetryString[_];
	String sumString(sum, HEX);

	// Add the checksum to the end
	telemetryString += cs_sep;
	telemetryString += sumString;

	// Send the data
	TELEMETRY_SERIAL.println(telemetryString);
}
