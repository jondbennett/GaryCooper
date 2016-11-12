
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ICommInterface.h"
#include "Telemetry.h"

static char *Telemetry_ItoA(int num, char* str, int base);
static char *Telemetry_DtoA(char *str, double num, int places);
static void appendChecksum(char *_pcBuffer);

CTelemetry::CTelemetry()
{
	m_commInterface = 0;
	m_commandTarget = 0;
}

CTelemetry::~CTelemetry()
{
}

void CTelemetry::setInterfaces(ICommunicationInterface *_commInterface,
				ITelemetry_CommandTarget *_cmdTarget)
{
	m_commInterface = _commInterface;
	m_commandTarget = _cmdTarget;
}

void CTelemetry::tick()
{
}

void CTelemetry::send(int _tag, double _value)
{

	char telemetryBuf[256];
	char numberBuf[256];

	// sprintf is not fully implemented on the Arduino
	strcpy(telemetryBuf, "$");
	Telemetry_ItoA(_tag, numberBuf, 10);
	strcat(telemetryBuf, numberBuf);
	strcat(telemetryBuf, ",");
	Telemetry_DtoA(numberBuf, _value, 2);
	strcat(telemetryBuf, numberBuf);

	appendChecksum(telemetryBuf);

	// Send the data
	if(m_commInterface)
		m_commInterface->puts(telemetryBuf);
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
					m_checksum ^= _c;
					// Fall through

				case '*':
					processTerm();

					// And start the next
					m_termOffset = 0;
					m_term[m_termOffset] = '\0';
					m_termNumber++;

					// Star also ends the current term, and it changes state to checksum processing
					if(_c == '*')
						m_state = TParser_S_ProcessingChecksum;
					break;

				default:
					m_checksum ^= _c;
					AddTermChar(_c);
					break;
			}
			break;

		// Checking the checksum term
		case TParser_S_ProcessingChecksum:
			if(_c == '\r')	// This ends the current string so do EOL processing
			{
				// First, verify the checksum
				if(m_termOffset == 2)
				{
					// Check the sentence checksum against ours
					unsigned char checksum = 16 * from_hex(m_term[0]) + from_hex(m_term[1]);
					if(checksum == m_checksum)
					{
						if(m_commandTarget)
							m_commandTarget->processCommand(m_cmdTag, m_cmdValue);
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
				AddTermChar(_c);
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
void CTelemetry::AddTermChar(unsigned char _c)
{
	if(m_termOffset < (TParser_TERMSIZE - 1))
	{
		m_term[m_termOffset++] = _c;
		m_term[m_termOffset] = '\0';
	}
}

void CTelemetry::processTerm()
{
	if(m_termNumber == 0)
		m_cmdTag = atoi(m_term);

	if(m_termNumber == 1)
		m_cmdValue = atof(m_term);
}


void CTelemetry::reset()
{
	m_state = TParser_S_WaitingForStart;

	m_termOffset = 0;
	m_term[m_termOffset] = '\0';
	m_termNumber = 0;

	m_checksum = 0;

	m_cmdTag = TParser_INVALID_CMD;
	m_cmdValue = 0.;
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

	double precision = (1.0)/(pow(10, (double)(places + 1)));

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
			for (i = 0, j = m-1; i<j; i++, j--)
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
    int end = length -1;
    while (start < end)
    {
        swap(*(str+start), *(str+end));
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
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    Telemetry_Reverse(str, i);

    return str;
}

static const char *gs_pcHex = "0123456789ABCDEF";
static void appendChecksum(char *_pcBuffer)
{
	if(_pcBuffer == 0)
		return;

	int iChecksum = 0;
	char cs1, cs2;
	int iLen = strlen(_pcBuffer);
	if(iLen < 2)
		return;

	// Append the checksum prototype and line terminators
	strcat(_pcBuffer, "*XX\r\n");

	// Calculate the checksum
	// Always skip the first character
	++_pcBuffer;
	for(const char *c = _pcBuffer; *c; c++)
	{
		if(*c == '*')
			break;

		iChecksum ^= (int)(*c);
	}

	// Move from integer to hex characters
	cs1 = gs_pcHex[((iChecksum & 0xF0) >> 4)];
	cs2 = gs_pcHex[(iChecksum & 0x0F)];

	// Fill in the checksum prototype with
	// the actual checksum characters
	_pcBuffer[iLen + 0] = cs1;
	_pcBuffer[iLen + 1] = cs2;
}
