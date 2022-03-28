#ifndef SHP_NETWORKS_ESPNOW_SERVER_H
#define SHP_NETWORKS_ESPNOW_SERVER_H


#define SHP_ENSPS_NONE						0
#define SHP_ENSPS_IN_PROGRESS			1



class ShpEspNowServer : public ShpEspNow
{
	public:

		ShpEspNowServer();

		virtual void init();
		virtual void loop();
		static void espNowServerOnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

		void sendSinglePacket(const uint8_t *peer_addr, const char *data, size_t len, uint8_t packetType = SHP_ENPT_ONE_DATA_PACKET, uint16_t packetIndex = 0);


		void pairingStart();
		void pairingNextTry();
		void pairingStop(bool successfuly);
		void sendDeviceCfg();
		virtual void onReceiveData(uint8_t packetType, String data);

	protected:

		uint8_t m_pairingStatus;
		uint8_t m_pairingCounter;
		unsigned long m_pairingNextTry;
		uint8_t m_serverMac[6] {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

		bool m_sendDeviceCfg;
		String m_sendDeviceCfgId;
		String m_sendDeviceMac;

};


class ShpEspNowServerIOPort : public ShpIOPort
{
	public:

		ShpEspNowServerIOPort();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);
		
	protected:

	ShpEspNowServer *m_espNowServer;

};


#endif
