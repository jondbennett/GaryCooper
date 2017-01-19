/////////////////////////////////////////////////////////////////////////
// A generic sliding data buffer; Functions as a FIFO
/////////////////////////////////////////////////////////////////////////
#ifndef SlidingBuf_h
#define SlidingBuf_h

// Initial buffer size and realloc growth
// NOTE: *** ALSO IMPLIES MAXIMUM LINE LENGTH FOR Gets()
#define CSLIDING_BUFFER_BLOCKSIZE	(512)

class CSlidingBuffer
{
protected:
	unsigned char *m_buf;
	unsigned int m_bufLen;
	unsigned int m_dataLen;

	bool m_canGrow;
	void grow(unsigned int _newSize);

public:
	CSlidingBuffer();
	virtual ~CSlidingBuffer();
	void setCanGrow(bool _canGrow)
	{
		m_canGrow = _canGrow;
	}
	int bytesAvailable()
	{
		return m_dataLen;
	}

	virtual unsigned int read(unsigned char *_buf, unsigned int _bufSize, bool _consume = true);
	virtual unsigned int write(const unsigned char *_buf, unsigned int _bufSize);

	void consume(unsigned int _consumeLen);	// Remove data from the head
	// of the buffer

	virtual int gets(char *_buf, int _bufSize);
	virtual bool puts(const char * _buf);
};

/////////////////////////////////////////////////////////////////////////
#endif
