/////////////////////////////////////////////////////////////////////////
// A "portable" sliding buffer. Manages a buffer of single-byte chars
// and can return data one line at a time. This object supports the
// reception of data in chunks through a socket or serial port an bundles
// it back into single lines.
/////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "SlidingBuf.h"

CSlidingBuffer::CSlidingBuffer()
{
	m_pBuf = 0;
	m_iBufLen = 0;
	m_iDataLen = 0;
	m_bCanGrow = true;
}

CSlidingBuffer::~CSlidingBuffer()
{
	if(m_pBuf) free(m_pBuf);
	m_pBuf = 0;
	m_iBufLen = 0;
	m_iDataLen = 0;
}

void CSlidingBuffer::grow(unsigned int _uiNewSize)
{

	// Don't be stupid, keep it!
	if(m_iBufLen > _uiNewSize)
		return;

	unsigned int bufSizeNeeded = 0;
	unsigned int nBlocks = 0;

	if(!m_bCanGrow)
	{
		if(m_pBuf == 0)
		{
			m_pBuf = (unsigned char *)malloc(CSLIDING_BUFFER_BLOCKSIZE);
			return;
		}
	}

	// I can grow, so figure out how large
	nBlocks = (_uiNewSize / CSLIDING_BUFFER_BLOCKSIZE) + 1;
	bufSizeNeeded = nBlocks * CSLIDING_BUFFER_BLOCKSIZE;

	// Enlarge the buffer
	if(!m_pBuf)
	{
		m_pBuf = (unsigned char *)malloc(bufSizeNeeded);
	}
	else
	{
		m_pBuf = (unsigned char *)realloc(m_pBuf, bufSizeNeeded);
	}

	m_iBufLen = bufSizeNeeded;
}

void CSlidingBuffer::consume(unsigned int _iConsumeLen)
{
	// Ignore stupid requests
	if(_iConsumeLen < 1) return;

	// If this would wipe the buffer, treat it
	// as a special case
	if(_iConsumeLen >= m_iDataLen)
	{
		m_iDataLen = 0;
		return;
	}

	m_iDataLen -= _iConsumeLen;
	memmove(m_pBuf, m_pBuf + _iConsumeLen, m_iDataLen);
}

unsigned int CSlidingBuffer::read(unsigned char *_pBuf, unsigned int _iBufSize, bool _bConsume)
{
	unsigned int amountToCopy = 0;

	amountToCopy = bytesAvailable();
	if(amountToCopy > (_iBufSize))
		amountToCopy = _iBufSize;

	if(amountToCopy <= 0)
		return 0;

	memmove(_pBuf, m_pBuf, amountToCopy);

	if(_bConsume)
		consume(amountToCopy);

	return amountToCopy;
}

unsigned int CSlidingBuffer::write(const unsigned char *_pBuf, unsigned int _iBufSize)
{
	unsigned int bufSizeNeeded = 0;
	unsigned int freeSpace = 0;
	unsigned int lenToAdd = 0;

	// Don't be stupid
	if(!_pBuf) return 0;
	if(_iBufSize <= 0) return 0;

	// Do I need to grow?
	bufSizeNeeded = m_iDataLen + _iBufSize;
	if(m_iBufLen < bufSizeNeeded)
		grow(bufSizeNeeded);

	// Now see how much I can add
	freeSpace = m_iBufLen - m_iDataLen;
	lenToAdd = (freeSpace < _iBufSize)?freeSpace : _iBufSize;

	if(lenToAdd == 0)
		lenToAdd = 0;

	// Now add the data to the end of the buffer
	memmove(m_pBuf + m_iDataLen, _pBuf, lenToAdd);
	m_iDataLen += lenToAdd;

	return lenToAdd;
}

#define SKIPWHITESPACE(c) {while(*c && isspace(*c)) ++c; }
int CSlidingBuffer::gets(char *_pBuf, int _iBufLen)
{
	unsigned char buf[CSLIDING_BUFFER_BLOCKSIZE];
	int len = 0;
	unsigned char *start = 0;
	unsigned char *end = 0;
	unsigned char *eol = 0;
	int lenToConsume = 0;
	int lineLen = 0;
	unsigned int iBytesAvailable = bytesAvailable();

	// Check for empty
	if(!iBytesAvailable)
		return 0;

	// Get the data
	len = read(buf, sizeof(buf) - 1, false);
	if(!len)
		return 0;

	// Terminate
	buf[len] = 0;

	// Check for eol

	eol = (unsigned char *)strstr(( char *)buf,"\n");

	if(eol == 0)
	{
		// If it is just running away, then
		// flush the buffer
		// and start waiting for a
		// newline
		if(iBytesAvailable == m_iBufLen)
		{
			consume(iBytesAvailable);
		}
		else
		{
			// There is a special case where we can get a null characters
			// into the buffer. If this happens then Gets will bind.
			// By dumping a null-terminated string without a newline
			// character this is detected and stopped
			int bufStrlen = strlen((const char *)buf);
			if((bufStrlen > 0) && (bufStrlen < len))
			{
				// It is just an ugly null terminated string
				// so eat it and it's null
				consume(bufStrlen + 1);
			}
			else
			{
				int iNullCount = 0;

				// It must be a bunch of nulls
				for(iNullCount = 0; iNullCount < len; ++iNullCount)
					if(buf[iNullCount] != '\0')
						break;

				consume(iNullCount);
			}
		}
		return 0;
	}

	// Figure out how much we are going
	// to consume
	lenToConsume = (eol - buf) + 1;

	// Eat leading whitespace
	start = buf;
	SKIPWHITESPACE(start);

	// Eat trailing whitespace
	end = eol;
	while(isspace(*end) && (end >= start))
	{
		*end = 0;
		--end;
	}

	// Is there anything left?
	lineLen = (end - start) + 1;
	if(lineLen <= 0)
	{
		consume(lenToConsume);
		return 0;
	}

	// Perhaps they are sending a null buffer to
	// query the length needed
	if(_pBuf == 0)
		return lineLen;

	// If the buffer is insufficient
	// then return -1 to note the error
	if(_iBufLen <= lineLen)
		return -1;

	// Copy the data
	strcpy(_pBuf,(char *)start);

	// Remove the requested data from
	// my buffer
	consume(lenToConsume);

	return lineLen;
}

bool CSlidingBuffer::puts(const char * _pBuf)
{
	if(!_pBuf)
		return false;

	write((unsigned char *)_pBuf, strlen(_pBuf));
	return true;
}
