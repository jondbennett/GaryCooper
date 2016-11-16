#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ICommInterface.h"
#include "Telemetry.h"

static char *Telemetry_ItoA(int num, char* str, int base);
static char *Telemetry_DtoA(char *str, double num, int places);

CTelemetry::CTelemetry()
{
	m_commInterface = 0;
	m_receiveTarget = 0;
}

CTelemetry::~CTelemetry()
{
}

void CTelemetry::setInterfaces(ICommunicationInterface *_commInterface,
							   ITelemetry_ReceiveTarget *_receiveTarget)
{
	m_commInterface = _commInterface;
	m_receiveTarget = _receiveTarget;
}

void CTelemetry::tick()
{
	if(!m_commInterface)
		return;

	while(m_commInterface->bytesInReceiveBuffer())
	{
		char parseBuff[CTelemetry_TERMSIZE + 1];

		int dataLen = m_commInterface->read((unsigned char *)parseBuff, sizeof(parseBuff));
		if(dataLen) parse((unsigned char *)parseBuff, dataLen);
	}
}

// Interface for sending data
void CTelemetry::transmissionStart()
{
	// Make sure we have a comm interface to send it
	if(!m_commInterface)
		return;

	m_transmitTermNumber = 0;
	m_transmitChecksum = 0;

	m_commInterface->puts("$");
	m_commInterface->tick();
}

void CTelemetry::sendTerm(const char *_value)
{
	// Make sure we have a comm interface to send it
	if(!m_commInterface)
		return;

	// Copy the data to the buffer, prepending a comma if needed
	char termBuf[CTelemetry_TERMSIZE * 2];
	termBuf[0] = '\0';
	if(m_transmitTermNumber)
		strcpy(termBuf, ",");
	strncat(termBuf, _value, sizeof(termBuf));

	// Calculate the checksum
	for(const char *c = termBuf; *c; c++)
		m_transmitChecksum ^= (int)(*c);

	// Bump term number
	m_transmitTermNumber++;

	// Finally, send the data
	m_commInterface->puts(termBuf);
	m_commInterface->tick();
}

void CTelemetry::sendTerm(int _value)
{
	char numberBuf[CTelemetry_TERMSIZE + 1];
	Telemetry_ItoA(_value, numberBuf, 10);
	sendTerm(numberBuf);
}

void CTelemetry::sendTerm(bool _value)
{
	sendTerm((_value) ? 1 : 0);
}

void CTelemetry::sendTerm(double _value)
{
	char numberBuf[CTelemetry_TERMSIZE + 1];
	Telemetry_DtoA(numberBuf, _value, 2);
	sendTerm(numberBuf);
}

static const char *s_hexChars = "0123456789ABCDEF";
void CTelemetry::transmissionEnd()
{
	// Make sure we have a comm interface to send it
	if(!m_commInterface)
		return;

	// Create the checksum string
	char termBuf[6];
	termBuf[0] = '*';
	termBuf[1] = s_hexChars[((m_transmitChecksum & 0xF0) >> 4)];
	termBuf[2] = s_hexChars[(m_transmitChecksum & 0x0F)];
	termBuf[3] = '\r';
	termBuf[4] = '\n';
	termBuf[5] = '\0';

	// Send it
	m_commInterface->puts(termBuf);
	m_commInterface->tick();
}

// =========================================================
// Returns true if any sentences were processed (data may have changed)
void CTelemetry::parse(const unsigned char _buf[], unsigned int _bufLen)
{
	unsigned int index;
	const unsigned char *cPtr = _buf;

	// Sanity
	if((_buf == 0) || (_bufLen == 0))
		return;

	// parse it one character at a time.
	for(index = 0; index < _bufLen; ++index)
		parseChar(*cPtr++);
}

// This method was inspired by Mikal Hart's TinyGPS
void CTelemetry::parseChar(unsigned char _c)
{
	// Screen out the newline character because we don't use it
	if(_c == '\n')
		return;

	// Extremely special case for very (I mean VERY) noisy data streams)
	if((m_state != TParser_S_WaitingForStart) &&
			(_c == '$'))
	{
		// We have received a '$' when we were not expecting one, so the data must be a mess.
		// We'll just reset the parser and continue as though this '$' is the start of the
		// sentence.
		m_state = TParser_S_WaitingForStart;
	}

	// How the characters are handled depends on which
	// state we are in.
	switch(m_state)
	{
	// Waiting for $
	case TParser_S_WaitingForStart:
		if(_c == '$')
		{
			// Prep for parsing terms
			reset();

			// Change state to term processing
			m_state = TParser_S_ParsingTerms;

			// Let command target know that we are starting
			if(m_receiveTarget)
				m_receiveTarget->startReception();
		}
		break;

	// Processing comma-separated terms
	case TParser_S_ParsingTerms:
		switch(_c)
		{
		// Comma and asterisk both end the current term, so we do some
		// common processing to reduce code size. However, asterisk causes
		// a state change
		case ',':
			// Process this term
			m_receiveChecksum ^= _c;
			// Fall through

		case '*':
			processTerm();

			// And start the next
			m_receiveTermOffset = 0;
			m_receiveTerm[m_receiveTermOffset] = '\0';
			m_receiveTermNumber++;

			// Star also ends the current term, and it changes state to checksum processing
			if(_c == '*')
				m_state = TParser_S_ProcessingChecksum;
			break;

		default:
			m_receiveChecksum ^= _c;
			addReceiveTermChar(_c);
			break;
		}
		break;

	// Checking the checksum term
	case TParser_S_ProcessingChecksum:
		if(_c == '\r')	// This ends the current string so do EOL processing
		{
			// First, verify the checksum
			if(m_receiveTermOffset == 2)
			{
				// Check the sentence checksum against ours
				unsigned char checksum = 16 * from_hex(m_receiveTerm[0]) + from_hex(m_receiveTerm[1]);
				if(m_receiveTarget)
				{
					if(checksum == m_receiveChecksum)
						m_receiveTarget->receiveChecksumCorrect();
					else
						m_receiveTarget->receiveChecksumError();
				}
			}

			// reset the parser
			reset();

			// And start looking for the next one
			m_state = TParser_S_WaitingForStart;
		}
		else
		{
			// Not the end of the string, so keep processing characters
			addReceiveTermChar(_c);
		}
		break;

	// Invalid state... we should never get here
	default:
		break;
	}

#ifdef CTelemetry_DEBUG
	// Line number tracking for diagnostics
	if(_c == '\r')
	{
		// Bump the line number
		m_TParser_LineNumber++;
	}
#endif
}

// Add character to accumulating term, but do not
// overrun the buffer
void CTelemetry::addReceiveTermChar(unsigned char _c)
{
	if(m_receiveTermOffset < (CTelemetry_TERMSIZE - 1))
	{
		m_receiveTerm[m_receiveTermOffset++] = _c;
		m_receiveTerm[m_receiveTermOffset] = '\0';
	}
}

void CTelemetry::processTerm()
{
	if(m_receiveTarget)
		m_receiveTarget->receiveTerm(m_receiveTermNumber, m_receiveTerm);
}


void CTelemetry::reset()
{
	m_state = TParser_S_WaitingForStart;

	m_receiveTermOffset = 0;
	m_receiveTerm[m_receiveTermOffset] = '\0';
	m_receiveTermNumber = 0;

	m_receiveChecksum = 0;
}

unsigned char CTelemetry::from_hex(unsigned char _a)
{
	if (_a >= 'A' && _a <= 'F')
		return _a - 'A' + 10;
	else if (_a >= 'a' && _a <= 'f')
		return _a - 'a' + 10;
	else
		return _a - '0';
}


/**
 * Double to ASCII
 * "Borrowed" from androider at
 * http://stackoverflow.com/questions/2302969/how-to-implement-char-ftoafloat-num-without-sprintf-library-function-i
 */
static char *Telemetry_DtoA(char *str, double num, int places)
{

	double precision = (1.0) / (pow(10, (double)(places + 1)));

	// handle special cases
	if (isnan(num))
	{
		strcpy(str, "nan");
	}
	else if (isinf(num))
	{
		strcpy(str, "inf");
	}
	else if (num == 0.0)
	{
		strcpy(str, "0");
	}
	else
	{
		int digit, m;
		int m1 = 0;
		char *c = str;
		int neg = (num < 0);
		if (neg)
		{
			num = -num;
		}
		// calculate magnitude
		m = log10(num);
		int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
		if (neg)
		{
			*(c++) = '-';
		}
		// set up for scientific notation
		if (useExp)
		{
			if (m < 0)
			{
				m -= 1.0;
			}
			num = num / pow(10.0, m);
			m1 = m;
			m = 0;
		}
		if (m < 1.0)
		{
			m = 0;
		}
		// convert the number
		while (num > precision || m >= 0)
		{
			double weight = pow(10.0, m);
			if (weight > 0 && !isinf(weight))
			{
				digit = floor(num / weight);
				num -= (digit * weight);
				*(c++) = '0' + digit;
			}
			if (m == 0 && num > 0)
			{
				*(c++) = '.';
			}
			m--;
		}
		if (useExp)
		{
			// convert the exponent
			int i, j;
			*(c++) = 'e';
			if (m1 > 0)
			{
				*(c++) = '+';
			}
			else
			{
				*(c++) = '-';
				m1 = -m1;
			}
			m = 0;
			while (m1 > 0)
			{
				*(c++) = '0' + m1 % 10;
				m1 /= 10;
				m++;
			}
			c -= m;
			for (i = 0, j = m - 1; i < j; i++, j--)
			{
				// swap without temporary
				c[i] ^= c[j];
				c[j] ^= c[i];
				c[i] ^= c[j];
			}
			c += m;
		}
		*(c) = '\0';
	}
	return str;
}

/* A C++ program to implement itoa() */

template<class T> inline
void swap(T &first, T &second)
{
	if (&first != &second)
	{
		T tmp = first;
		first = second;
		second = tmp;
	}
}

/* A utility function to reverse a string  */
static void Telemetry_Reverse(char str[], int length)
{
	int start = 0;
	int end = length - 1;
	while (start < end)
	{
		swap(*(str + start), *(str + end));
		start++;
		end--;
	}
}

// Implementation of itoa()
static char* Telemetry_ItoA(int num, char* str, int base)
{
	int i = 0;
	bool isNegative = false;

	/* Handle 0 explicitely, otherwise empty string is printed for 0 */
	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	// In standard itoa(), negative numbers are handled only with
	// base 10. Otherwise numbers are considered unsigned.
	if (num < 0 && base == 10)
	{
		isNegative = true;
		num = -num;
	}

	// Process individual digits
	while (num != 0)
	{
		int rem = num % base;
		str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num = num / base;
	}

	// If number is negative, append '-'
	if (isNegative)
		str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	Telemetry_Reverse(str, i);

	return str;
}

