#include <string.h>

#include <Arduino.h>

#include <SaveController.h>
#include <GPSParser.h>

#include "ICommInterface.h"
#include "Telemetry.h"
#include "TelemetryTags.h"
#include "MilliTimer.h"

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
	DEBUG_SERIAL.println(F("CCommand - Command Starting"));
#endif
	m_term0 = telemetry_tag_invalid;
	m_term1 = 0.;
}

void CCommand::receiveTerm(int _index, const char *_value)
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	String valStr(_value);
	DEBUG_SERIAL.print(F("CCommand - Adding term: "));
	DEBUG_SERIAL.print(_index);
	DEBUG_SERIAL.print(F(" = "));
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
	DEBUG_SERIAL.println(F("CCommand - received checksum correct."));
#endif
	processCommand(m_term0, m_term1);
}

void CCommand::receiveChecksumError()
{
#ifdef DEBUG_COMMAND_PROCESSOR_INTERFACE
	DEBUG_SERIAL.println(F("CCommand - received checksum error."));
#endif
}

void CCommand::processCommand(int _tag, double _value)
{
	// Don't process commands until after we know the protocol version
	if(_tag == telemetry_command_version)
	{
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - set telemetry version: "));
		DEBUG_SERIAL.println(_value);
#endif

		// Only accept valid versions instead of blind assignment
		if(_value == 1.)
		{
			m_version = TELEMETRY_VERSION_01;
			ackCommand(_tag, _value);
		}
	}
	else
	{
		// We only accept commands after a command version has
		// been established
		if(m_version == TELEMETRY_VERSION_INVALID)
		{
#ifdef DEBUG_COMMAND_PROCESSOR
			DEBUG_SERIAL.println(F("CCommand - command rejected (version not set)."));
#endif
			nakCommand(_tag, _value, telemetry_cmd_response_nak_version_not_set);
		}

		// Only process after we have a valid version
		if(m_version == TELEMETRY_VERSION_01)
			processCommand_V1(_tag, _value);
	}
}

void CCommand::processCommand_V1(int _tag, double _value)
{
	int sunriseOffset = (int)_value;
	int sunsetOffset = (int)_value;
	bool lightOn = (_value > 0.) ? true : false;
	int stuckDoorDelay = (int) _value;
	doorCommandE doorCommand = (_value > 0.) ? doorCommand_open : doorCommand_close;

	telemetrycommandResponseE commandResponse;

	// Act on the command
	switch(_tag)
	{
	case telemetry_command_setSunriseOffset:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - setSunriseOffset: "));
		DEBUG_SERIAL.println(sunriseOffset);
#endif
		commandResponse = g_doorController.setSunriseOffset(sunriseOffset);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();

			ackCommand(_tag, _value);

			g_doorController.checkTime();
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_setSunsetOffset:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - setSunsetOffset: "));
		DEBUG_SERIAL.println(sunsetOffset);
#endif
		commandResponse = g_doorController.setSunsetOffset(sunsetOffset);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();

			ackCommand(_tag, _value);

			g_doorController.checkTime();
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_setMinimumDayLength:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - setMinimumDayLength: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_lightController.setMinimumDayLength(_value);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();

			ackCommand(_tag, _value);

			g_lightController.checkTime();
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_setExtraIlluminationMorning:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - setExtraLightTimeMorning: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_lightController.setExtraLightTimeMorning(_value);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();

			ackCommand(_tag, _value);

			g_lightController.checkTime();
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_setExtraIlluminationEvening:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - setExtraLightTimeEvening: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_lightController.setExtraLightTimeEvening(_value);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();

			ackCommand(_tag, _value);

			g_lightController.checkTime();
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_forceDoor:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - force door command: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_doorController.command(doorCommand);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			ackCommand(_tag, _value);
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_forceLight:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - force light command: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_lightController.command(lightOn);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			ackCommand(_tag, _value);
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_setStuckDoorDelay:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - set stuck door delay: "));
		DEBUG_SERIAL.println(_value);
#endif
		commandResponse = g_doorController.setStuckDoorDelay(stuckDoorDelay);
		if(commandResponse == telemetry_cmd_response_ack)
		{
			saveSettings();
			loadSettings();
			ackCommand(_tag, _value);
		}
		else
		{
			nakCommand(_tag, _value, commandResponse);
		}
		break;

	case telemetry_command_loadDefaults:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.println(F("CCommand - *** RESET ALL SETTINGS ***"));
#endif
		g_saveController.updateHeader(0xfe);
		loadSettings();
		ackCommand(_tag, _value);
		break;

	default:
#ifdef DEBUG_COMMAND_PROCESSOR
		DEBUG_SERIAL.print(F("CCommand - Invalid command tag: "));
		DEBUG_SERIAL.print(_tag);
		DEBUG_SERIAL.print(F(" value: "));
		DEBUG_SERIAL.println(_value);
#endif
		nakCommand(_tag, _value, telemetry_cmd_response_nak_invalid_command);
		break;
	}
}

void CCommand::ackCommand(int _tag, double _value)
{
#ifdef DEBUG_COMMAND_PROCESSOR
	DEBUG_SERIAL.print(F("CCommand - acking Tag: "));
	DEBUG_SERIAL.print(_tag);
	DEBUG_SERIAL.print(F("  Value: "));
	DEBUG_SERIAL.println(_value);
#endif
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_command_ack);
	g_telemetry.sendTerm((int)_tag);
	g_telemetry.sendTerm((double)_value);
	g_telemetry.transmissionEnd();
}

void CCommand::nakCommand(int _tag, double _value, telemetrycommandResponseE _reason)
{
#ifdef DEBUG_COMMAND_PROCESSOR
	DEBUG_SERIAL.print(F("CCommand - *** Nacking Tag: "));
	DEBUG_SERIAL.print(_tag);
	DEBUG_SERIAL.print(F("  Value: "));
	DEBUG_SERIAL.print(_value);
	DEBUG_SERIAL.print(F("  Reason: "));
	DEBUG_SERIAL.println(_reason);
#endif
	g_telemetry.transmissionStart();
	g_telemetry.sendTerm(telemetry_tag_command_nak);
	g_telemetry.sendTerm((int)_tag);
	g_telemetry.sendTerm((double)_value);
	g_telemetry.sendTerm((int)_reason);
	g_telemetry.transmissionEnd();
}
