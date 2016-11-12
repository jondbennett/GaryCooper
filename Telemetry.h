#ifndef Telemetry_h
#define Telemetry_h

// Pure base class for commands received by the telemetry module
class ITelemetry_CommandTarget
{
public:
	virtual void processCommand(int _tag, double _value) = 0;
};


// State machine states
typedef enum
{
	TParser_S_WaitingForStart = 0,		// Waiting for $
	TParser_S_ParsingTerms,			// Processing comma-separated terms
	TParser_S_ProcessingChecksum		// Checking the checksum term
} TParser_State_T;


// Largest distance between commas and such
#define TParser_TERMSIZE		(16)
#define TParser_INVALID_CMD		(-1)

// The telemetry module
class CTelemetry
{
protected:

	// Stuff for processing commands from the house
	ICommunicationInterface *m_commInterface;
	ITelemetry_CommandTarget *m_commandTarget;

	// Current state machine state
	TParser_State_T m_state;

	// Location to accumulate data as the parse
	// progresses. Mostly for the stuff between
	// commas
	char m_term[TParser_TERMSIZE];
	int m_termOffset;
	int m_termNumber;

	// Running checksum
	unsigned char m_checksum;

	// The actual command (from process term)
	int m_cmdTag;
	double m_cmdValue;

	// Parser processing
	void parseChar(unsigned char _c);
	void AddTermChar(unsigned char _c);

	void processTerm();		// Returns true when valid and recognized sentence is complete
	unsigned char from_hex(unsigned char _a);

	void reset();

	// Returns true if any sentences were processed (data may have changed)
	void parse(const unsigned char _buf[], unsigned int _bufLen);

public:

	CTelemetry();
	virtual ~CTelemetry();

	void setInterfaces(ICommunicationInterface *_commInterface,
				ITelemetry_CommandTarget *_cmdTarget);

	void tick();

	void send(int _tag, double _value);
};

#endif
