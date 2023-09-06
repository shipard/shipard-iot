#ifndef SHP_CLIENT_CAN_H
#define SHP_CLIENT_CAN_H

#define MAX_DATA_CAN_BUF_LEN 100

#define BUS_CAN_CONTROL_QUEUE_LEN 6

#define BUS_CAN_ITEM_TYPE_NONE 0
#define BUS_CAN_ITEM_TYPE_WRITE_BUFFER 1

#define BUS_CAN_WRITE_BUFFER_MAX_LEN 32

#define CLIENT_CAN_INCOMING_QUEUE_LEN 2


#define CLIENT_CAN_CONTROL_QUEUE_LEN 16
#define CLIENT_CAN_QIT_NONE 									0
#define CLIENT_CAN_QIT_PUBLISH 								1
#define CLIENT_CAN_QIT_SEND_SIMPLE_PACKET 		2
#define CLIENT_CAN_QIT_SEND_FW_BLOCK_REQUEST 	3

#include <ESP32CAN.h>
#include <CAN_config.h>
#include <routers/can/canCore.h>


#define ccsNeedDeviceId 		0
#define ccsWaitForDeviceId 	1
#define ccsNeedCFG			 		2
#define ccsWaitForCFG 			3
#define ccsDownloadCFG 			4
#define ccsRunning 					5


#define qsInProgress 8

class ShpOTAUpdateCAN;


class ShpClientCAN
{
	public:

		ShpClientCAN();

		virtual void init();
		virtual void loop();

		void publish(const char *payload, const char *topic, const uint8_t packetCmd = ROUTER_CAN_CMD_PUBLISH_FROM_DEVICE);
		void doFwUpgradeRequest(String payload);
		void addDownloadFWQueueItem(uint8_t blockNumber, int startAfter);

	protected:

		void sendRequestDeviceId();
		void sendRequestCfg();

		void doIncomingPacket(uint32_t msgId, uint8_t dataLen, uint8_t *data);
		void runQueueItem(int i);
		void doQueueItemStep(int i);

	private:

		gpio_num_t m_rxPin;
		gpio_num_t m_txPin;

		int m_deviceId;

		uint8_t m_clientState;
		unsigned long m_clientStateRetryMillis;


		CAN_device_t m_CAN_cfg;
		uint8_t m_clientHWId[CAN_DEVICE_HW_ID_LEN]; // MAC address

		struct IncomingQueueItem
		{
			uint8_t packetCmd;
			uint8_t packetType;
			String data;
			uint16_t stringPacketBlockNumber;
			uint16_t stringPacketBlocksCount;
			int sendedLen;
		};

		IncomingQueueItem m_incomingQueue[CLIENT_CAN_INCOMING_QUEUE_LEN];

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;

			int8_t queueItemType;

			int deviceId;

			uint8_t packetDataLen;
			uint8_t packetData[ROUTER_CAN_STD_PACKET_LEN];

			uint8_t packetCmd;
			uint8_t packetType;
			String data;
			uint16_t stringPacketBlockNumber;
			uint16_t stringPacketBlocksCount;
		};

		int m_queueRequests;
		QueueItem m_queue[CLIENT_CAN_CONTROL_QUEUE_LEN];

		ShpOTAUpdateCAN *m_OTAUpdate;
};


#endif
