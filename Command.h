#ifndef COMMAND_H
#define COMMAND_H

class CCommand : public ITelemetry_CommandTarget
{
protected:

	int m_version;
	void processCommand_V1(int _tag, double _value);

public:
	CCommand();
	virtual ~CCommand();

	virtual void processCommand(int _tag, double _value);

};

#endif
