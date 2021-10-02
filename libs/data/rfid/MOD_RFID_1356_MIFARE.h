#ifndef SHP_MOD_RFID_1356_MIFARE_H
#define SHP_MOD_RFID_1356_MIFARE_H

#define MAX_MOD_RFID_1356_MIFARE_BUF_LEN 80

class ShpMODRfid1356Mifare : public ShpIOPort 
{
	public:
		
		ShpMODRfid1356Mifare();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		//void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

	protected:

		void sendData(const char *data);

	private:

		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;
		bool m_revertCode;
		unsigned long m_sameTagIdTimeout;
		String m_portIdBuzzer;
		
		char m_lastCardId[MAX_MOD_RFID_1356_MIFARE_BUF_LEN + 1];
		unsigned long m_lastTagIdReadMillis;


		uint8_t m_currentDimLevel;
		uint8_t m_defaultDimLevel;

		HardwareSerial *m_hwSerial;

		char m_buffer[MAX_MOD_RFID_1356_MIFARE_BUF_LEN];
		int m_sbCnt;
		int m_bufNeedSend;

};


#endif
