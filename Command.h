#ifndef COMMAND_H
#define COMMAND_H

class CCommand : public ITelemetry_ReceiveTarget
{
protected:

	int m_version;

	int m_term0;
	double m_term1;

	void processCommand(int _tag, double _value);

	void ackCommand(int _tag, double _value);
	void nakCommand(int _tag, double _value, telemetrycommandResponseE _reason);

	void processCommand_V1(int _tag, double _value);

public:
	CCommand();
	virtual ~CCommand();

	virtual void startReception();
	virtual void receiveTerm(int _index, const char *_value);
	virtual void receiveChecksumCorrect();
	virtual void receiveChecksumError();
};

#endif
