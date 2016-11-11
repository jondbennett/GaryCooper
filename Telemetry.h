#ifndef Telemetry_h
#define Telemetry_h

#define TELEMETRY_VALUE_NULL			(0.)

typedef enum
{
	telemetry_tag_error = -1,
	telemetry_tag_startup = 0,

	telemetry_tag_GPSNSats,
	telemetry_tag_lat,
	telemetry_tag_lon,

	telemetry_tag_currentTime,
	telemetry_tag_doorOpenTime,
	telemetry_tag_doorCloseTime,

	telemetry_tag_morningLightOnTime,
	telemetry_tag_morningLightOffTime,

	telemetry_tag_eveningLightOnTime,
	telemetry_tag_eveningLightOffTime,

	telemetry_tag_door_state,
	telemetry_tag_light_state,
} telemetryTagE;

typedef enum
{
	telemetry_error_GPS_no_data = 0,
	telemetry_error_GPS_not_locked,
	telemetry_error_GPS_bad_data,

	telemetry_error_door_state,
} telemetryErrorT;

void telemetrySend(telemetryTagE _tag, double _value);

#endif
