#ifndef SHP_CONTROL_BISTRELAY_H
#define SHP_CONTROL_BISTRELAY_H


#define BISTRELAY_CONTROL_QUEUE_LEN 10

#define BISTRELAY_LEDMODE_NONE 	0
#define BISTRELAY_LEDMODE_BIN 	1
#define BISTRELAY_LEDMODE_PWM 	2


class ShpControlBistRelay : public ShpIOPort
{
	public:

		ShpControlBistRelay();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		virtual void onMessage(byte* payload, unsigned int length, const char* subCmd);

	protected:

		int8_t m_pin1;
		const char *m_pin1ExpPortId;
		ShpGpioExpander *m_pin1GpioExpander;

		int8_t m_pin2;
		const char *m_pin2ExpPortId;
		ShpGpioExpander *m_pin2GpioExpander;

		int8_t m_stateOn;
		int8_t m_stateOff;
		int8_t m_stateCurrent;
		int8_t m_stateSaved;

		int8_t m_ledMode;
		int8_t m_pinLed;
		uint8_t m_ledBr;
		uint8_t m_pwmChannel;

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			int8_t state;
			long duration;
		};


		int m_queueRequests;
		QueueItem m_queue[BISTRELAY_CONTROL_QUEUE_LEN];
		unsigned long m_doEndPulseAfter;
		unsigned long m_saveStateAfter;
		uint32_t m_saveInterval;

		void addQueueItem(int8_t state, unsigned long startAfter);
		void runQueueItem(int i);

		void setState(uint8_t state);
		void setPinState1(uint8_t value);
		void setPinState2(uint8_t value);
		void endPulse();

};


#endif
