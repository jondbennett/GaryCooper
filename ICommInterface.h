////////////////////////////////////////////////////////////
// Pure virtual class for implementing generic communications
////////////////////////////////////////////////////////////
#ifndef ICCOMMUNICATIONINTERFACE_H
#define ICCOMMUNICATIONINTERFACE_H

////////////////////////////////////////////////////////////
// More Description and notes
////////////////////////////////////////////////////////////
class ICommunicationInterface
{
public:
	virtual unsigned int read(unsigned char *_pBuf, unsigned int _iBufSize, bool _bConsume = true) = 0;
	virtual unsigned int write(const unsigned char *_pBuf, unsigned int _iBufSize) = 0;
	virtual int getError() = 0;

	virtual int bytesInReceiveBuffer() = 0;
	virtual int bytesInTransmitBuffer() = 0;

	virtual int gets(char *_pBuf, int _iBufLen) = 0;
	virtual bool puts(const char * _pBuf) = 0;

	virtual void tick() = 0;	// Support for polled environments
};

#endif
