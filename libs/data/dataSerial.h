#ifndef SHP_DATA_SERIAL_H
#define SHP_DATA_SERIAL_H

#define MAX_DATA_SERIAL_BUF_LEN 100

// -- shipard cfg item `mac.devices.serialLineMode` --> board value
#define SERIAL_MODE_MAP_CNT 24
static uint32_t SERIAL_MODE_MAP[] = {
	SERIAL_8N1, SERIAL_8N2, SERIAL_8E1, SERIAL_8E2, SERIAL_8O1, SERIAL_8O2,
	SERIAL_7N1, SERIAL_7N2, SERIAL_7E1, SERIAL_7E2, SERIAL_7O1, SERIAL_7O2,
	SERIAL_6N1, SERIAL_6N2, SERIAL_6E1, SERIAL_6E2, SERIAL_6O1, SERIAL_6O2,
	SERIAL_5N1, SERIAL_5N2, SERIAL_5E1, SERIAL_5E2, SERIAL_5O1, SERIAL_5O2
};

// -- shipard cfg item `mac.devices.serialLineSpeed` --> board value
#define SERIAL_SPEED_MAP_CNT 9
static unsigned long SERIAL_SPEED_MAP[SERIAL_SPEED_MAP_CNT] = {0, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200};

class ShpDataSerial : public ShpIOPort 
{
	public:
		ShpDataSerial();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	private:
		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;
		int m_telnetPort;

		HardwareSerial *m_hwSerial;
		ShpTelnet *m_telnet;

		char m_buffer[MAX_DATA_SERIAL_BUF_LEN];
		int m_sbCnt;
		int m_bufNeedSend;
};


#endif
