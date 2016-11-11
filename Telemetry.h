#ifndef Telemetry_h
#define Telemetry_h

#define TELEMETRY_VALUE_NULL			(0.)

typedef enum
{
	telemetry_tag_error = -1,
	telemetry_tag_startup = 0,
	telemetry_tag_door_state,
	telemetry_tag_light_state,
} telemetryTagE;

typedef enum
{
	telemetry_error_GPS_no_data = 0,
	telemetry_error_GPS_not_locked,

	telemetry_error_door_state,
} telemetryErrorT;

void telemetrySend(telemetryTagE _tag, double _value);

#endif
