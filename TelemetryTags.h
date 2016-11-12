#ifndef TelemetryTags_h
#define TelemetryTags_h

// Telemetry and command stuff
#define TELEMETRY_VERSION_INVALID		(0)
#define TELEMETRY_VERSION				(1)

// Telemetry tags sent FROM the coop controller
typedef enum
{
	telemetry_tag_error = -1,

	telemetry_tag_version = 0,

	telemetry_tag_GPSLockStatus,
	telemetry_tag_GPSNSats,
	telemetry_tag_lat,
	telemetry_tag_lon,

	telemetry_tag_year,
	telemetry_tag_month,
	telemetry_tag_day,
	telemetry_tag_currentTime,


	telemetry_tag_doorOpenTime,
	telemetry_tag_doorCloseTime,
	telemetry_tag_door_state,

	telemetry_tag_morningLightOnTime,
	telemetry_tag_morningLightOffTime,
	telemetry_tag_eveningLightOnTime,
	telemetry_tag_eveningLightOffTime,
	telemetry_tag_light_state,
} telemetryTagE;

// Error types sent from the coop controller:
// NOTE: telemetry tag will be telemetry_tag_error
typedef enum
{
	telemetry_error_GPS_no_data = 0,
	telemetry_error_GPS_bad_data,

	telemetry_error_door_state,

	telemetry_error_receivedInvalidCommand,

} telemetryErrorE;

typedef enum
{
	telemetry_command_version = 100,

	telemetry_command_setSunriseType,
	telemetry_command_setSunsetType,

	telemetry_command_setMinimumDayLength,
	telemetry_command_setExtraIllumination,

	telemetry_command_forceDoor,
	telemetry_command_forceLight,
}
telemetryCommandE;

#endif
