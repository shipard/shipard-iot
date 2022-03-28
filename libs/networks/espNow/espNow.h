#ifndef SHP_NETWORKS_ESPNOW_H
#define SHP_NETWORKS_ESPNOW_H


#define SHP_ENPT_ONE_DATA_PACKET 		16
#define SHP_ENPT_MESSAGE 						32
#define SHP_ENPT_PAIR_REQUEST 			64
#define SHP_ENPT_PAIR_RESPONSE 			128
#define SHP_ENPT_CFG_REQUEST 				144
#define SHP_ENPT_CFG_RESPONSE 			148


#define SHP_ENS_IDLE											0
#define SHP_WAIT_FOR_CLIENT_CFG						1
#define SHP_WAIT_FOR_CLIENT_PAIR					2
#define SHP_WAIT_DO_CLIENT_PAIR_REQUEST		3
#define SHP_ENS_UNINITIALIZED			 			 99

String shpMacToStr(const uint8_t* mac);
void shpStrToMac(const char* str, uint8_t* mac);

#define SHP_PACKET_HEADER_LEN			 		7
#define SHP_MAX_PACKET_DATA_LEN			243

typedef struct __attribute__((__packed__)) shp_en_packet_t {
  uint8_t type;
  uint16_t packetIndex;
	uint16_t totalPackets;
	uint16_t dataLen;
	char data[SHP_MAX_PACKET_DATA_LEN];
};


#define SHP_ENSQS_NONE							0
#define SHP_ENSQS_WAIT_FOR_START		1
#define SHP_ENSQS_IN_PROGRESS				2

#define SHP_ENSQ_LEN								10

typedef struct shp_en_send_queue_item_t {
	uint8_t state;
	uint8_t macAddr[6] {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t packetType;
	uint8_t nextPacketIdx;
	uint8_t totalPackets;
	String data;
};


#define SHP_ENSQR_NONE							0
#define SHP_ENSQR_IN_PROGRESS				1
#define SHP_ENSQR_WAIT_FOR_RECEIVE	2

#define SHP_ENRQ_LEN								20

typedef struct shp_en_recv_queue_item_t {
	uint8_t state;
	uint8_t macAddr[6] {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	uint8_t type;
	uint8_t lastPacketIndex;
	uint8_t totalPackets;
	String data;
};

class ShpEspNow
{
	public:

		ShpEspNow();

		void sendData(String &data, const uint8_t packetType, const char *toAddress);
		void sendNextDataPacket();
		void sendNextDataPacketItem(int i);

		void receiveData(const uint8_t *peer_addr, shp_en_packet_t *packet);

		virtual void onReceiveData(uint8_t packetType, String data);
		virtual void loop();

	protected:

		void doReceiveData();

		uint8_t m_anyDataToSend;
		uint8_t m_anyDataToReceive;
		shp_en_send_queue_item_t m_sendQueue[SHP_ENSQ_LEN];
		shp_en_recv_queue_item_t m_recvQueue[SHP_ENRQ_LEN];

		uint8_t m_pairingMac[6] {0x92, 0x9E, 0x98, 0x03, 0x4E, 0x9E}; // 92:9e:98:03:4e:9e

};

#endif
