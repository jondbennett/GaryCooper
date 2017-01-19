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
	m_buf = 0;
	m_bufLen = 0;
	m_dataLen = 0;
	m_canGrow = true;
}

CSlidingBuffer::~CSlidingBuffer()
{
	if(m_buf) free(m_buf);
	m_buf = 0;
	m_bufLen = 0;
	m_dataLen = 0;
}

void CSlidingBuffer::grow(unsigned int _newSize)
{

	// Don't be stupid, keep it!
	if(m_bufLen > _newSize)
		return;

	unsigned int bufSizeNeeded = 0;
	unsigned int nBlocks = 0;

	if(!m_canGrow)
	{
		if(m_buf == 0)
		{
			m_buf = (unsigned char *)malloc(CSLIDING_BUFFER_BLOCKSIZE);
			return;
		}
	}

	// I can grow, so figure out how large
	nBlocks = (_newSize / CSLIDING_BUFFER_BLOCKSIZE) + 1;
	bufSizeNeeded = nBlocks * CSLIDING_BUFFER_BLOCKSIZE;

	// Enlarge the buffer
	if(!m_buf)
	{
		m_buf = (unsigned char *)malloc(bufSizeNeeded);
	}
	else
	{
		m_buf = (unsigned char *)realloc(m_buf, bufSizeNeeded);
	}

	m_bufLen = bufSizeNeeded;
}

void CSlidingBuffer::consume(unsigned int _consumeLen)
{
	// Ignore stupid requests
	if(_consumeLen < 1) return;

	// If this would wipe the buffer, treat it
	// as a special case
	if(_consumeLen >= m_dataLen)
	{
		m_dataLen = 0;
		return;
	}

	m_dataLen -= _consumeLen;
	memmove(m_buf, m_buf + _consumeLen, m_dataLen);
}

unsigned int CSlidingBuffer::read(unsigned char *_buf, unsigned int _bufSize, bool _consume)
{
	unsigned int amountToCopy = 0;

	amountToCopy = bytesAvailable();
	if(amountToCopy > (_bufSize))
		amountToCopy = _bufSize;

	if(amountToCopy <= 0)
		return 0;

	memmove(_buf, m_buf, amountToCopy);

	if(_consume)
		consume(amountToCopy);

	return amountToCopy;
}

unsigned int CSlidingBuffer::write(const unsigned char *_buf, unsigned int _bufSize)
{
	unsigned int bufSizeNeeded = 0;
	unsigned int freeSpace = 0;
	unsigned int lenToAdd = 0;

	// Don't be stupid
	if(!_buf) return 0;
	if(_bufSize <= 0) return 0;

	// Do I need to grow?
	bufSizeNeeded = m_dataLen + _bufSize;
	if(m_bufLen < bufSizeNeeded)
		grow(bufSizeNeeded);

	// Now see how much I can add
	freeSpace = m_bufLen - m_dataLen;
	lenToAdd = (freeSpace < _bufSize) ? freeSpace : _bufSize;

	if(lenToAdd == 0)
		lenToAdd = 0;

	// Now add the data to the end of the buffer
	memmove(m_buf + m_dataLen, _buf, lenToAdd);
	m_dataLen += lenToAdd;

	return lenToAdd;
}

#define SKIPWHITESPACE(c) {while(*c && isspace(*c)) ++c; }
int CSlidingBuffer::gets(char *_buf, int _bufSize)
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

	eol = (unsigned char *)strstr(( char *)buf, "\n");

	if(eol == 0)
	{
		// If it is just running away, then
		// flush the buffer
		// and start waiting for a
		// newline
		if(iBytesAvailable == m_bufLen)
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
	if(_buf == 0)
		return lineLen;

	// If the buffer is insufficient
	// then return -1 to note the error
	if(_bufSize <= lineLen)
		return -1;

	// Copy the data
	strcpy(_buf, (char *)start);

	// Remove the requested data from
	// my buffer
	consume(lenToConsume);

	return lineLen;
}

bool CSlidingBuffer::puts(const char * _buf)
{
	if(!_buf)
		return false;

	write((unsigned char *)_buf, strlen(_buf));
	return true;
}
