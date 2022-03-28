#ifndef SHP_NETWORKS_ESPNOW_CLIENT_H
#define SHP_NETWORKS_ESPNOW_CLIENT_H


class ShpEspNowClient : public ShpEspNow
{
	public:

		ShpEspNowClient();



		//static void espNowClientOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

		virtual void init();
		virtual void loop();
		//virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);
		
		void writeIotBoxConfig(String data);

		bool readServerAddress();
		void writeServerAddress();
		void doPair();
		void doPairRequestInit(shp_en_packet_t *packet);
		void doPairRequestRun();
		void setServerAddress(const char *address);


		void sendSinglePacket(const char *data, size_t len, uint8_t packetType = SHP_ENPT_ONE_DATA_PACKET, uint16_t packetIndex = 0);

		static void espNowClientOnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

		virtual void onReceiveData(uint8_t packetType, String data);

	public:

		uint8_t m_mode;
		String m_serverAddress;
		uint8_t m_clientMac[6] {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

};


#endif
