#ifndef SHP_BUS_CAN_H
#define SHP_BUS_CAN_H

#define MAX_DATA_CAN_BUF_LEN 100

#define BUS_CAN_CONTROL_QUEUE_LEN 6

#define BUS_CAN_ITEM_TYPE_NONE 0
#define BUS_CAN_ITEM_TYPE_WRITE_BUFFER 1

#define BUS_CAN_WRITE_BUFFER_MAX_LEN 32

#include <ESP32CAN.h>
#include <CAN_config.h>


class ShpBusCAN : public ShpIOPort
{
	public:
		ShpBusCAN();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		void addQueueItemWrite(int8_t bufferLen, const byte* buffer, unsigned long startAfter);
		void runQueueItem(int i);

	private:

		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;

		//HardwareSerial *m_hwSerial;

		CAN_device_t m_CAN_cfg;

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			int8_t queueItemType;
			uint8_t bufferLen;
			uint8_t buffer[BUS_RS485_WRITE_BUFFER_MAX_LEN];
		};


		int m_queueRequests;
		QueueItem m_queue[BUS_RS485_CONTROL_QUEUE_LEN];

		char m_buffer[MAX_DATA_RS485_BUF_LEN];
		int m_sbCnt;
		int m_bufNeedSend;
};


#endif
