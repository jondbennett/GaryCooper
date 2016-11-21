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
			nakCommand(_tag, telemetry_cmd_nak_version_not_set);
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

void CCommand::nakCommand(int _tag, telemetrycommandNakT _reason)
{
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_command_nak);
	g_telemetry.sendTerm(_tag);
	g_telemetry.sendTerm(_reason);
	g_telemetry.transmissionEnd();
}

void CCommand::processCommand_V1(int _tag, double _value)
{
	int sunriseOffset = (int)_value;
	int sunsetOffset = (int)_value;
	bool lightOn = (_value > 0.) ? true : false;
	doorController_doorStateE doorState = (doorController_doorStateE)_value;

	// Act on the command
	switch(_tag)
	{
	case telemetry_command_setSunriseOffset:
		if(g_doorController.setSunriseOffset(sunriseOffset))
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set sunrise offset: "));
			DEBUG_SERIAL.println(sunriseOffset);
#endif
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set sunrise offset: FAILED - "));
			DEBUG_SERIAL.println(sunriseOffset);
#endif
			nakCommand(_tag, telemetry_cmd_nak_invalid_value);
		}
		break;

	case telemetry_command_setSunsetOffset:
		if(g_doorController.setSunsetOffset(sunsetOffset))
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set sunset offset: "));
			DEBUG_SERIAL.println(sunsetOffset);
#endif
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set sunset offset: FAILED - "));
			DEBUG_SERIAL.println(sunsetOffset);
#endif
			nakCommand(_tag, telemetry_cmd_nak_invalid_value);
		}
		break;

	case telemetry_command_setMinimumDayLength:
		if(g_lightController.setMinimumDayLength(_value))
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set minimum day length: "));
			DEBUG_SERIAL.println(_value);
#endif
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.print(PMS("CCommand - set minimum day length: FAILED - "));
			DEBUG_SERIAL.println(_value);
#endif
			nakCommand(_tag, telemetry_cmd_nak_invalid_value);
		}
		break;

	case telemetry_command_setExtraIllumination:
		if(g_lightController.setExtraLightTime(_value))
		{
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set extra light time: "));
		DEBUG_SERIAL.println(_value);
#endif
			saveSettings();
			loadSettings();
			ackCommand(_tag);
		}
		else
		{
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(PMS("CCommand - set extra light time: FAILED - "));
		DEBUG_SERIAL.println(_value);
#endif
			nakCommand(_tag, telemetry_cmd_nak_invalid_value);
		}
		break;

	case telemetry_command_forceDoor:
		// Make sure we got a good state
		if((doorState != doorController_doorOpen) && (doorState != doorController_doorClosed))
		{
			nakCommand(_tag, telemetry_cmd_nak_invalid_value);
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
		nakCommand(_tag, telemetry_cmd_nak_invalid_command);
		break;
	}
}


