#ifndef SHP_DATA_RFIDPN532_H
#define SHP_DATA_RFIDPN532_H

#include <PN532.h>
#include <PN532_I2C.h>
#include <PN532_HSU.h>


#define MAX_DATA_RFID_PN532_TAG_LEN_BYTES 7 
#define MAX_DATA_RFID_PN532_BUF_LEN (MAX_DATA_RFID_PN532_TAG_LEN_BYTES*2+1)
#define PN532_MODE_I2C 0
#define PN532_MODE_HSU 1

class ShpDataRFIDPN532 : public ShpIOPort
{
	public:
		ShpDataRFIDPN532();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:
		
		void readTag();

	public:

		int8_t m_mode;
		int8_t m_pinRX;
		int8_t m_pinTX;
		unsigned long m_sameTagIdTimeout;
		String m_portIdBuzzer;
		
		char m_lastCardId[MAX_DATA_RFID_PN532_BUF_LEN];
		unsigned long m_lastTagIdReadMillis;

		HardwareSerial *m_hwSerial;
		PN532_HSU *m_pn532hsu;
		const char *m_busPortId;
		ShpBusI2C *m_bus;
		PN532_I2C *m_pn532i2c;
		PN532 *m_nfc;

};


#endif
