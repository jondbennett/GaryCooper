
#include <Arduino.h>

#include <PMS.h>

#include "Telemetry.h"

void telemetrySetup()
{
	TELEMETRY_SERIAL_PORT.begin(TELEMETRY_BAUD_RATE);
}

void telemetrySend(telemetryTagE _tag, double _value)
{
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
