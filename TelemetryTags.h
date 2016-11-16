#ifndef TelemetryTags_h
#define TelemetryTags_h

// Telemetry and command stuff
#define TELEMETRY_VERSION_INVALID		(0)
#define TELEMETRY_VERSION_01			(1)

// Telemetry tags sent FROM the coop controller
typedef enum
{
	telemetry_tag_error = -1,

	telemetry_tag_version = 0,

	telemetry_tag_GPSStatus,	// Lock, nSats, lat, lon

	telemetry_tag_date_time,	// Year, month, day, current time as flost

	telemetry_tag_door_config,	// Sunrise and Sunset types

	telemetry_tag_door_info,	// Open time, close time (UTC) float, door state - 0 = closed, 1 = open

	telemetry_tag_light_config,	// min day length, extra illumination time

	telemetry_tag_light_info,	// Morning on / off times , evening on / off times,, state - 0 = off, 1 = on

	telemetry_tag_command_ack = 50,	// Send to ack a command (value is command tag)

} telemetryTagE;

// Error types sent FROM the coop controller:
// NOTE: telemetry tag will be telemetry_tag_error
typedef enum
{
	telemetry_error_GPS_no_data = 1,
	telemetry_error_GPS_bad_data,
	telemetry_error_GPS_not_locked,

	telemetry_error_suncalc_invalid_time,

	telemetry_error_no_door_motor,
	telemetry_error_door_motor_unknown_state,
	telemetry_error_door_not_responding,

	telemetry_error_version_not_set,
	telemetry_error_received_invalid_command,
	telemetry_error_received_invalid_command_value,
} telemetryErrorE;

// Commands sent TO the coop controller
typedef enum
{
	telemetry_command_version = 100,

	telemetry_command_setSunriseType,
	telemetry_command_setSunsetType,

	telemetry_command_setMinimumDayLength,
	telemetry_command_setExtraIllumination,

	telemetry_command_forceDoor,
	telemetry_command_forceLight,

	telemetry_command_loadDefaults
}
telemetryCommandE;

#endif
