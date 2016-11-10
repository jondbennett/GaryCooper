////////////////////////////////////////////////////////////
// Serial port handler for the AGV
////////////////////////////////////////////////////////////
#ifndef Comm_Arduino_h
#define Comm_Arduino_h

////////////////////////////////////////////////////////////
// Arduino comm interface (serial port)
////////////////////////////////////////////////////////////
#define CComm_Arduino_SERIAL_PORT_CLOSED	(-1)

class CComm_Arduino
{
	private:
		int m_serialID;
		CSlidingBuffer m_transmitBuf;
		CSlidingBuffer m_receiveBuf;

	public:
		CComm_Arduino()
		{
			m_serialID = CComm_Arduino_SERIAL_PORT_CLOSED;
			m_transmitBuf.setCanGrow(false);
			m_receiveBuf.setCanGrow(false);
		}

		~CComm_Arduino()
		{
			if(m_serialID >= 0)
				close();
		}

		unsigned int read(unsigned char *_buf, unsigned int _bufSize, bool _consume = true)
		{
			unsigned int uiAmountRead = m_receiveBuf.read(_buf, _bufSize, _consume);
			return uiAmountRead;
		}

		unsigned int write(const unsigned char *_buf, unsigned int _bufSize)
		{
			return m_transmitBuf.write(_buf, _bufSize);
		}

		int getError()
		{
			return 0;
		}

		int bytesInReceiveBuffer()
		{
			return m_receiveBuf.bytesAvailable();
		}

		int bytesInTransmitBuffer()
		{
			return m_transmitBuf.bytesAvailable();
		}

		int gets(char *_buf, int _bufSize)
		{
			return m_receiveBuf.gets(_buf, _bufSize);
		}

		bool puts(const char * _buf)
		{
			return m_transmitBuf.puts(_buf);
		}

		bool open(int _iSerialID, unsigned long _buad)
		{
			if(m_serialID >= 0)
				close();

			switch(_iSerialID)
			{
				case 0:
					m_serialID = 0;
					Serial.begin(_buad);
					break;

				case 1:
					m_serialID = 1;
					Serial1.begin(_buad);
					break;

				case 2:
					m_serialID = 2;
					Serial2.begin(_buad);
					break;

				case 3:
					m_serialID = 3;
					Serial3.begin(_buad);
					break;

				default:
					m_serialID = CComm_Arduino_SERIAL_PORT_CLOSED;
					break;
			}

			if(m_serialID >= 0)
                return true;

            return false;
		}

		void close()
		{
			switch(m_serialID)
			{
				case 0:
					m_serialID = 0;
					Serial.end();
					break;

				case 1:
					m_serialID = 1;
					Serial1.end();
					break;

				case 2:
					m_serialID = 2;
					Serial2.end();
					break;

				case 3:
					m_serialID = 3;
					Serial3.end();
					break;

				default:
					m_serialID = CComm_Arduino_SERIAL_PORT_CLOSED;
					break;
			}

			m_serialID = CComm_Arduino_SERIAL_PORT_CLOSED;
		}

		void tick()
		{
			unsigned byteCount;
			unsigned char data[512];

			switch(m_serialID)
			{
				case 0:
					byteCount = Serial.available();
					if(byteCount)
					{
						Serial.readBytes(data, byteCount);
						m_receiveBuf.write(data, byteCount);
					}

					byteCount = m_transmitBuf.bytesAvailable();
					if(byteCount > sizeof(data))
						byteCount = sizeof(data);
					m_transmitBuf.read(data, byteCount);
					Serial.write(data, byteCount);
					break;

				case 1:
					byteCount = Serial1.available();
					if(byteCount)
					{
						Serial1.readBytes(data, byteCount);
						m_receiveBuf.write(data, byteCount);
					}

					byteCount = m_transmitBuf.bytesAvailable();
					if(byteCount > sizeof(data))
						byteCount = sizeof(data);
					m_transmitBuf.read(data, byteCount);
					Serial1.write(data, byteCount);
					break;

				case 2:
					byteCount = Serial2.available();
					if(byteCount)
					{
						Serial2.readBytes(data, byteCount);
						m_receiveBuf.write(data, byteCount);
					}

					byteCount = m_transmitBuf.bytesAvailable();
					if(byteCount > sizeof(data))
						byteCount = sizeof(data);
					m_transmitBuf.read(data, byteCount);
					Serial2.write(data, byteCount);
					break;

				case 3:
					byteCount = Serial3.available();
					if(byteCount)
					{
						Serial3.readBytes(data, byteCount);
						m_receiveBuf.write(data, byteCount);
					}

					byteCount = m_transmitBuf.bytesAvailable();
					if(byteCount > sizeof(data))
						byteCount = sizeof(data);
					m_transmitBuf.read(data, byteCount);
					Serial3.write(data, byteCount);
					break;

				default:
					break;
			}
		}

		bool wantsTick()
		{
			if(m_transmitBuf.bytesAvailable())
				return true;

			switch(m_serialID)
			{
				case 0:
					if(Serial.available())
						return true;
					break;
				case 1:
					if(Serial1.available())
						return true;
					break;
				case 2:
					if(Serial2.available())
						return true;
					break;
				case 3:
					if(Serial3.available())
						return true;
					break;

				default:
					return false;
			}
			return false;
		}
};

#endif
