#ifndef SHP_CONTROL_HBRIDGE_H
#define SHP_CONTROL_HBRIDGE_H


#define HBRIDGE_CONTROL_QUEUE_LEN 10


class ShpControlHBridge : public ShpIOPort 
{
	public:

		ShpControlHBridge();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

	protected:

		int8_t m_pin1;
		const char *m_pin1ExpPortId;
		ShpGpioExpander *m_pin1GpioExpander;

		int8_t m_pin2;
		const char *m_pin2ExpPortId;
		ShpGpioExpander *m_pin2GpioExpander;



		int8_t m_stateCurrent;

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			int8_t state;
			long duration;
		};


		int m_queueRequests;
		QueueItem m_queue[HBRIDGE_CONTROL_QUEUE_LEN];
		uint8_t m_switchToState;
		unsigned long m_switchToStateAfter;

		void addQueueItem(int8_t state, long duration, unsigned long startAfter);
		void runQueueItem(int i);

		void setState(uint8_t state);
		void setPinState1(uint8_t value);
		void setPinState2(uint8_t value);


};


#endif
