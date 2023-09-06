#ifndef SHP_BUS_RS485_H
#define SHP_BUS_RS485_H

#define MAX_DATA_RS485_BUF_LEN 100

#define BUS_RS485_CONTROL_QUEUE_LEN 6

#define BUS_RS485_ITEM_TYPE_NONE 0
#define BUS_RS485_ITEM_TYPE_WRITE_BUFFER 1

#define BUS_RS485_WRITE_BUFFER_MAX_LEN 32

class ShpBusRS485 : public ShpIOPort
{
	public:
		ShpBusRS485();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		uint16_t modbusCalcCRC(uint8_t *buffer, uint8_t bufferLen);

		void addQueueItemWrite(int8_t bufferLen, const byte* buffer, unsigned long startAfter);
		void runQueueItem(int i);

	private:

		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;
		int8_t m_controlPin;

		HardwareSerial *m_hwSerial;

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
