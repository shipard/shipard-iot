#ifndef SHP_DATA_RFIDPN125KHZ_H
#define SHP_DATA_RFIDPN125KHZ_H



//#define MAX_DATA_RFID_PN532_TAG_LEN_BYTES 7 
#define MAX_DATA_RFID_125KHZ_BUF_LEN 14

class ShpDataRFID125KHZ : public ShpIOPort
{
	public:
		ShpDataRFID125KHZ();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	protected:
		
		//void readTag();

		long extract_tag(uint8_t *buffer);
		long hexstr_to_value(char *str, unsigned int length);

	protected:

		int8_t m_pinRX;
		int8_t m_pinTX;
		unsigned long m_sameTagIdTimeout;
		

		uint8_t m_buffer[MAX_DATA_RFID_125KHZ_BUF_LEN];

		String m_portIdBuzzer;
		
		long m_lastTagId;
		unsigned long m_lastTagIdReadMillis;

		HardwareSerial *m_hwSerial;

};


#endif
