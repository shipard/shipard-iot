#ifndef SHP_INPUT_BINARY_H
#define SHP_INPUT_BINARY_H


class ShpInputBinary : public ShpIOPort 
{
	public:

		ShpInputBinary();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

	protected:

		int8_t m_pin;
		long m_timeout;
		uint8_t m_detectedValue;
		
		int8_t m_lastValue;
		unsigned long m_lastChangeMillis;
		boolean m_disable;
		boolean m_waitForChange;
		boolean m_needSend;


	protected:
		
		void onPinChange(int pin);

};


#endif
