#ifndef SHP_CONTROL_LEVEL_H
#define SHP_CONTROL_LEVEL_H


#define SWITCH_CONTROL_LEVEL_QUEUE_LEN 32

class ShpControlLevel : public ShpIOPort
{
	public:

		ShpControlLevel();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void onMessage(byte* payload, unsigned int length, const char* subCmd);

	protected:

		int8_t m_pin;
		uint32_t m_defaultValue;
		uint8_t m_startMax;
		int8_t m_pwmChannel;
		uint32_t m_pinValueMin;
		uint32_t m_pinValueMax;
		int8_t m_steps;
		uint32_t m_pinValueCurrent;
		uint32_t m_pinValueLast;

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			int8_t pinNumber;
			uint32_t pinValue;
		};


		int m_queueRequests;
		QueueItem m_queue[SWITCH_CONTROL_LEVEL_QUEUE_LEN];

		void addQueueItemFromMessage(byte* payload, unsigned int length);
		void addQueueItem(int8_t pinNumber, uint32_t pinValue, unsigned long startAfter);
		void runQueueItem(int i);

};


#endif
