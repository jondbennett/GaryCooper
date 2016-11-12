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
		unsigned char *m_pBuf;
		unsigned int m_iBufLen;
		unsigned int m_iDataLen;

		bool m_bCanGrow;
		void grow(unsigned int _uiNewSize);

	public:
		CSlidingBuffer();
		virtual ~CSlidingBuffer();
		void setCanGrow(bool _bCanGrow) { m_bCanGrow = _bCanGrow; }
		int bytesAvailable() { return m_iDataLen; }

		virtual unsigned int read(unsigned char *_pBuf, unsigned int _iBufSize, bool _bConsume = true);
		virtual unsigned int write(const unsigned char *_pBuf, unsigned int _iBufSize);

		void consume(unsigned int _iConsumeLen);	// Remove data from the head
													// of the buffer

		virtual int gets(char *_pBuf, int _iBufLen);
		virtual bool puts(const char * _pBuf);
};

/////////////////////////////////////////////////////////////////////////
#endif
