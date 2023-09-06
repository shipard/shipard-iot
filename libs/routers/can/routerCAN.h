#ifndef SHP_ROUTER_CAN_H
#define SHP_ROUTER_CAN_H

#include <ESP32CAN.h>
#include <CAN_config.h>
#include "canCore.h"

#define ROUTER_CAN_CONTROL_QUEUE_LEN 64

#define BUS_CAN_ITEM_TYPE_NONE 0
#define BUS_CAN_ITEM_TYPE_WRITE_BUFFER 1

#define BUS_CAN_WRITE_BUFFER_MAX_LEN 32


#define qsInProgress 8



//String shpMacToStr(const uint8_t* mac);
//void shpStrToMac(const char* str, uint8_t* mac);

#define ROUTER_CAN_FIRST_DEVICE_ID 64

#define ROUTER_CAN_QIT_NONE				 								0
#define ROUTER_CAN_QIT_SEND_SIMPLE_PACKET 				1
#define ROUTER_CAN_QIT_GET_DEVICE_CFG 						2
#define ROUTER_CAN_QIT_SEND_STRING_DATA		 				3
#define ROUTER_CAN_QIT_UPLOAD_FIRMWARE_BEGIN			4
#define ROUTER_CAN_QIT_UPLOAD_FIRMWARE_SEND_BLOCK	5

class ShpRouterCAN : public ShpIOPort
{
	public:
		ShpRouterCAN();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		virtual void onMessage(byte* payload, unsigned int length, const char* subCmd);
		virtual void routeMessage(const char* topic, byte* payload, unsigned int length);

		void addQueueItem_SendSimplePacket(int deviceId, int8_t packetDataLen, const byte* packetData, unsigned long startAfter);
		void addQueueItem_DeviceCfgRequest(int deviceId, unsigned long startAfter);
		void addQueueItem_FWUpgradeRequest(int deviceId, const char *payload);
		void addQueueItem_FWUpgradeSendBlock(int deviceId, uint8_t blockNumber);

		void runQueueItem(int i);

	protected:

		void doIncomingPacket(uint32_t msgId, uint8_t dataLen, uint8_t *data);

		int searchDeviceNdxByHWId(const uint8_t *hwId);
		int getDeviceId(const uint8_t *hwId);

		uint8_t doQueueItemStep(int i);

	private:

		gpio_num_t m_rxPin;
		gpio_num_t m_txPin;
		int m_deviceMsgId;

		struct DeviceItem
		{
			uint8_t clientHWId[CAN_DEVICE_HW_ID_LEN];
			const char *topic;
			ShpOTAUpdateSlowSender *otaSender;
		};

		int m_devicesCount;
		int m_maxDevicesCount;
		DeviceItem *m_devices;

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

			long nextRunAfter;
			//ShpOTAUpdateSlowSender *otaSender;
		};

		int m_queueRequests;
		QueueItem m_queue[ROUTER_CAN_CONTROL_QUEUE_LEN];

		struct IncomingQueueItem
		{
			uint8_t packetCmd;
			uint8_t packetType;
			String data;
			uint16_t stringPacketBlockNumber;
			uint16_t stringPacketBlocksCount;
			int sendedLen;
		};

		IncomingQueueItem *m_incomingQueue;


		CAN_device_t m_CAN_cfg;
};

#endif
