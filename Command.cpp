#include <string.h>

#include <Arduino.h>

#include <SaveController.h>
#include <GPSParser.h>
#include <PMS.h>

#include "ICommInterface.h"
#include "Telemetry.h"
#include "TelemetryTags.h"

#include "Pins.h"
#include "SunCalc.h"
#include "DoorController.h"
#include "LightController.h"
#include "BeepController.h"
#include "GaryCooper.h"

#include "Command.h"

CCommand::CCommand()
{
	m_version = TELEMETRY_VERSION_INVALID;
}

CCommand::~CCommand()
{
}

void CCommand::startReception()
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	DEBUG_SERIAL.println(PMS("CCommand - Command Starting"));
#endif
}

void CCommand::receiveTerm(int _index, const char *_value)
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	String valStr(_value);
	DEBUG_SERIAL.print(PMS("CCommand - Adding term: "));
	DEBUG_SERIAL.print(_index);
	DEBUG_SERIAL.print(PMS(" = "));
	DEBUG_SERIAL.println(valStr);
#endif

	if(!_value || !strlen(_value))
		return;

	switch(_index)
	{
	case 0:
		m_term0 = atoi(_value);
		break;

	case 1:
		m_term1 = atof(_value);
		break;

	default:
		break;
	}
}

void CCommand::receiveChecksumCorrect()
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	DEBUG_SERIAL.println(PMS("CCommand - received checksum correct."));
#endif
	processCommand(m_term0, m_term1);
}

void CCommand::receiveChecksumError()
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	DEBUG_SERIAL.println(PMS("CCommand - received checksum error."));
#endif
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

		// Only accept valid versions instead of blind assignment
		if(_value == 1.)
		{
			m_version = 1;
			ackCommand(_tag);
		}
	}
	else
	{
		// We only accept commands after a command version has
		// been established
		if(m_version == TELEMETRY_VERSION_INVALID)
		{
			reportError(telemetry_error_version_not_set);
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.println(PMS("CCommand - command rejected (version not set)."));
#endif
		}

		// Only process after we have a valid version
		if(m_version == TELEMETRY_VERSION_01) processCommand_V1(_tag, _value);
	}
}

void CCommand::ackCommand(int _tag)
{
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_command_ack);
	g_telemetry.sendTerm(_tag);
	g_telemetry.transmissionEnd();
}

void CCommand::processCommand_V1(int _tag, double _value)
{
	eSunrise_Sunset_T sunriseType = (eSunrise_Sunset_T)_value;
	eSunrise_Sunset_T sunsetType = (eSunrise_Sunset_T)_value;
	bool lightOn = (_value > 0.) ? true : false;
	doorController_doorStateE doorState = (doorController_doorStateE)_value;

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
		if(sunriseType >= srsst_astronomical && sunriseType <= srsst_common)
		{
			g_doorController.setSunriseType(sunriseType);
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
			reportError(telemetry_error_received_invalid_command_value);
		}
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
		if(sunsetType >= srsst_astronomical && sunsetType <= srsst_common)
		{
			g_doorController.setSunsetType(sunsetType);
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
			reportError(telemetry_error_received_invalid_command_value);
		}
		break;

	case telemetry_command_setMinimumDayLength:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set minimum day length: "));
		DEBUG_SERIAL.println(_value);
#endif
		g_lightController.setMinimumDayLength(_value);
		saveSettings();
		loadSettings();
		ackCommand(_tag);
		break;

	case telemetry_command_setExtraIllumination:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set extra light time: "));
		DEBUG_SERIAL.println(_value);
#endif
		g_lightController.setExtraLightTime(_value);
		saveSettings();
		loadSettings();
		ackCommand(_tag);
		break;

	case telemetry_command_forceDoor:
		// Make sure we got a good state
		if((doorState != doorController_doorOpen) && (doorState != doorController_doorClosed))
		{
			reportError(telemetry_error_received_invalid_command_value);
			break;
		}

#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - force door: "));
		DEBUG_SERIAL.println((doorState == doorController_doorOpen) ? PMS("Open.") : PMS("Closed."));
#endif
		g_doorController.setDoorState(doorState);
		ackCommand(_tag);
		break;

	case telemetry_command_forceLight:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - force light: "));
		DEBUG_SERIAL.println((lightOn) ? PMS("On.") : PMS("Off."));
#endif
		g_lightController.setLightOn(lightOn);
		ackCommand(_tag);
		break;

	case telemetry_command_loadDefaults:
		g_saveController.updateHeader(0xfe);
		loadSettings();
		ackCommand(_tag);
		break;
	default:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - Invalid command tag: "));
		DEBUG_SERIAL.print(_tag);
		DEBUG_SERIAL.print(PMS(" Value: "));
		DEBUG_SERIAL.println(_value);
#endif
		reportError(telemetry_error_received_invalid_command);
		break;
	}
}


