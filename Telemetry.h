#ifndef Telemetry_h
#define Telemetry_h

// Pure base class for data received by the telemetry module
class ITelemetry_ReceiveTarget
{
public:
	virtual void startReception() = 0;
	virtual void receiveTerm(int _index, const char *_value) = 0;
	virtual void receiveChecksumCorrect() = 0;
	virtual void receiveChecksumError() = 0;
};


// State machine states
typedef enum
{
	TParser_S_WaitingForStart = 0,		// Waiting for $
	TParser_S_ParsingTerms,			// Processing comma-separated terms
	TParser_S_ProcessingChecksum		// Checking the checksum term
} TParser_State_T;


// Largest distance between commas and such
#define CTelemetry_TERMSIZE		(16)

// The telemetry module
class CTelemetry
{
protected:

	// Stuff for processing commands from the house
	ICommunicationInterface *m_commInterface;
	ITelemetry_ReceiveTarget *m_receiveTarget;

	// Current state machine state
	TParser_State_T m_state;

	// Location to accumulate data as the parse
	// progresses. Mostly for the stuff between
	// commas
	char m_receiveTerm[CTelemetry_TERMSIZE];
	int m_receiveTermOffset;
	int m_receiveTermNumber;

	// Running checksum
	unsigned char m_receiveChecksum;

	// Parser processing
	void parseChar(unsigned char _c);
	void addReceiveTermChar(unsigned char _c);

	void processTerm();		// Returns true when valid and recognized sentence is complete
	unsigned char from_hex(unsigned char _a);

	void reset();


	// Members for sending data
	int m_transmitTermNumber;
	int m_transmitChecksum;

public:

	CTelemetry();
	virtual ~CTelemetry();

	void setInterfaces(ICommunicationInterface *_commInterface,
					   ITelemetry_ReceiveTarget *_receiveTarget);

	// Returns true if any sentences were processed (data may have changed)
	void parse(const unsigned char _buf[], unsigned int _bufLen);

	void tick();

	// Interface for sending data
	void transmissionStart();
	void sendTerm(const char *_value);
	void sendTerm(int _value);
	void sendTerm(bool _value);
	void sendTerm(double _value);
	void transmissionEnd();
};

#endif
