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

	telemetry_tag_date_time,	// Year, month, day, as ints, current time as float (UTC)

	telemetry_tag_sun_times,	// Sunrise and Sunset times as float (UTC)

	telemetry_tag_door_config,	// Sunrise open and Sunset close offsets

	telemetry_tag_door_info,	// Open time, close time (UTC) float, door state - 0 = closed, 1 = open

	telemetry_tag_light_config,	// min day length, extra illumination time

	telemetry_tag_light_info,	// Morning on / off times , evening on / off times,, state - 0 = off, 1 = on

	telemetry_tag_command_ack = 50,	// Send to ack a command (value is command tag)
	telemetry_tag_command_nak = 51,	// Send to nak a command (values are command tag, reason)

} telemetryTagE;

// Reasons that a command could be nak'd
typedef enum
{
	telemetry_cmd_nak_version_not_set = 1,
	telemetry_cmd_nak_invalid_command,
	telemetry_cmd_nak_invalid_value,

} telemetrycommandNakT;

// Error types sent FROM the coop controller:
// NOTE: telemetry tag will be telemetry_tag_error
typedef enum
{
	telemetry_error_no_error 						= 0,
	telemetry_error_GPS_no_data						= (1 << 0),
	telemetry_error_GPS_bad_data					= (1 << 1),
	telemetry_error_GPS_not_locked					= (1 << 2),
	telemetry_error_suncalc_invalid_time			= (1 << 3),
	telemetry_error_no_door_motor					= (1 << 4),
	telemetry_error_door_motor_unknown_state		= (1 << 5),
	telemetry_error_door_not_responding				= (1 << 6),

} telemetryErrorE;

// Commands sent TO the coop controller
typedef enum
{
	telemetry_command_version = 100,

	telemetry_command_setSunriseOffset,
	telemetry_command_setSunsetOffset,

	telemetry_command_setMinimumDayLength,
	telemetry_command_setExtraIllumination,

	telemetry_command_forceDoor,
	telemetry_command_forceLight,

	telemetry_command_loadDefaults
}
telemetryCommandE;

#define GARY_COOPER_DOOR_MAX_TIME_OFFSET	(120)	// Sunrise / sunset +/- two hours in minutes


#define GARY_COOPER_LIGHT_MAX_DAY_LENGTH	(16.)
#define GARY_COOPER_LIGHT_MAX_EXTRA	(60.)

#define TELEMETRY_BAUD_RATE		(115200)

#endif
