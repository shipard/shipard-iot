#ifndef SHP_INPUT_COUNTER_H
#define SHP_INPUT_COUNTER_H

// counter value type
#define shpInputCouter_t int64_t
// sprintf format string
#define shpInputCouter_f "%lld"
// max ascii string size
#define shpInputCouter_a 32


#define shpCntrUseCoef_NO 0
#define shpCntrUseCoef_DIVIDE 1
#define shpCntrUseCoef_MULTIPLY 2

class ShpInputCounter : public ShpIOPort 
{
	public:

		ShpInputCounter();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);
		virtual void shutdown();

	protected:

		int8_t m_pin;
		boolean m_publishCntr;
		boolean m_publishPPS;
		boolean m_publishPPM;
		uint8_t m_useCoef;
		int m_coef;
		long m_timeoutOnToOff;
		int m_publishDelta;
		int m_checkIntervalOn;

		shpInputCouter_t m_valueCurrent;
		shpInputCouter_t m_valueSaved;
		shpInputCouter_t m_valueSent;

		unsigned long m_intervalSave;
		unsigned long m_intervalSend;
		unsigned long m_intervalCheck;

		unsigned long m_nextSaveCheck;
		unsigned long m_nextSendCheck;
		unsigned long m_nextCheck;

		shpInputCouter_t m_lastOnValue;
		uint8_t m_lastOnState;
		unsigned long m_nextCheckOn;
		unsigned long m_lastOffMillis;

		uint8_t m_lastPPSSec;
		uint8_t m_secsInMin = 0;
		shpInputCouter_t m_lastPPSValue;
		shpInputCouter_t m_pps[60];
		int m_lastPublishPPS;
		int m_lastPublishPPM;

		unsigned long m_sendTimeout;
		unsigned long m_lastSendMillisPPS;
		unsigned long m_lastSendMillisPPM;

		String m_valueTopicOn;
		String m_valueTopicPPS;
		String m_valueTopicPPM;
		String m_prefsId;

	protected:
		
		void checkPPS();
		void onPinChange(int pin);
		shpInputCouter_t loadValue();
		void saveValue(shpInputCouter_t value);
		void save();
		void send();

};


#endif
