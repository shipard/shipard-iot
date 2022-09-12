#ifndef SHP_CONTROL_BINARY_H
#define SHP_CONTROL_BINARY_H


#define SWITCH_CONTROL_QUEUE_LEN 32
#define BINARY_CONTROL_SCENARIO_LEN 16

#define qsFree 0
#define qsLocked 1
#define qsDoIt 2
#define qsRunning 3

#define qbcitSetValue 0
#define qbcitScenario 1


class ShpControlBinary : public ShpIOPort
{
	public:

		ShpControlBinary();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

	protected:

		int8_t m_pin;
		bool m_reverse;
		const char *m_pinExpPortId;
		#ifdef SHP_GPIO_EXPANDER_I2C_H
		ShpGpioExpander *m_gpioExpander;
		#endif
		int8_t m_PinStateOn;
		int8_t m_PinStateOff;
		int8_t m_PinStateCurrent;

		struct QueueScenarioItem {
			int8_t pinState;
			int duration;
		};

		struct QueueItem
		{
			int8_t qState;
			int8_t qType;
			unsigned long startAfter;

			int8_t itemType;
			int8_t pinState;

			QueueScenarioItem scenario[BINARY_CONTROL_SCENARIO_LEN];
			int8_t scenarioLen;
			int8_t scenarioPos;
			int8_t endScenarioPinState;

			int scenarioDurationLen;
			unsigned long scenarioDurationExpire;
			int scenarioRepeatCount;
			int scenarioRepeatPos;
		};


		int m_queueRequests;
		QueueItem m_queue[SWITCH_CONTROL_QUEUE_LEN];

		void setPinState(uint8_t value);

		void addQueueItemFromMessage(const char* topic, byte* payload, unsigned int length);
		void addQueueItemPressButton(int duration, unsigned long startAfter, bool unpush);
		void addQueueItemFlash(int durations[], int durationsCnt, int repeatCnt, int totalDuration, unsigned long startAfter);

		void addQueueItem(int8_t pinState, unsigned long startAfter);
		void runQueueItem(int i);

};


#endif
