#include <Arduino.h>

#include <SaveController.h>
#include <GPSParser.h>
#include <PMS.h>

#include "ICommInterface.h"
#include "Telemetry.h"
#include "TelemetryTags.h"

#include "GaryCooper.h"
#include "Pins.h"

#include "SunCalc.h"
#include "DoorController.h"
#include "LightController.h"

#include "Command.h"

extern CDoorController g_doorController;
extern CLightController g_lightController;
extern CTelemetry g_telemetry;

CCommand::CCommand()
{
	m_version = TELEMETRY_VERSION_INVALID;
}

CCommand::~CCommand()
{
}

void CCommand::processCommand(int _tag, double _value)
{
	// Don't process commands until after we know the protocol version
	if(_tag == telemetry_command_version)
	{
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set telemetry version: "));
		DEBUG_SERIAL.println(_value);
#endif
		// Check for valid versions instead of blind assignment
		if(_value == 1.) m_version = 1;
	}
	else
	{
		// Only process after we have a valid version
		if(m_version == 1) processCommand_V1(_tag, _value);
	}
}

void CCommand::processCommand_V1(int _tag, double _value)
{
	eSunrise_Sunset_T sunriseType = (eSunrise_Sunset_T)_value;
	eSunrise_Sunset_T sunsetType = (eSunrise_Sunset_T)_value;
	bool force = (_value > 0.) ? true : false;

	// Act on the command
	switch(_tag)
	{
	case telemetry_command_setSunriseType:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set sunrise type: "));
		DEBUG_SERIAL.println((sunriseType == srsst_astronomical) ? PMS("astronomical.") :
							 (sunriseType == srsst_nautical) ? PMS("nautical.") :
							 (sunriseType == srsst_civil) ? PMS("civil.") :
							 (sunriseType == srsst_common) ? PMS("common.") :
							 PMS("** INVALID **")
							);
#endif
		g_doorController.setSunriseType(sunriseType);
		break;

	case telemetry_command_setSunsetType:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set sunset type: "));
		DEBUG_SERIAL.println((sunsetType == srsst_astronomical) ? PMS("astronomical.") :
							 (sunsetType == srsst_nautical) ? PMS("nautical.") :
							 (sunsetType == srsst_civil) ? PMS("civil.") :
							 (sunsetType == srsst_common) ? PMS("common.") :
							 PMS("** INVALID **")
							);
#endif
		g_doorController.setSunsetType(sunsetType);
		break;

	case telemetry_command_setMinimumDayLength:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set minimum day length: "));
		DEBUG_SERIAL.println(_value);
#endif
		g_lightController.setMinimumDayLength(_value);
		break;

	case telemetry_command_setExtraIllumination:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set extra light time: "));
		DEBUG_SERIAL.println(_value);
#endif
		g_lightController.setExtraLightTime(_value);
		break;

	case telemetry_command_forceDoor:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - force door: "));
		DEBUG_SERIAL.println((force) ? PMS("Open") : PMS("Closed"));
#endif
		g_doorController.toggleDoorState();
		break;

	case telemetry_command_forceLight:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - force light: "));
		DEBUG_SERIAL.println((force) ? PMS("On") : PMS("Off"));
#endif
		g_lightController.setLightOn(force);
		break;

	default:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - Invalid command tag: "));
		DEBUG_SERIAL.print(_tag);
		DEBUG_SERIAL.print(PMS(" Value: "));
		DEBUG_SERIAL.println(_value);
#endif
		g_telemetry.send(telemetry_tag_error, telemetry_error_receivedInvalidCommand);
		break;
	}
}
