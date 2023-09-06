#ifndef SHP_INPUT_BINARY_H
#define SHP_INPUT_BINARY_H

#define IBMODE_INT 0
#define IBMODE_GPIO_EXP 1

class ShpInputBinary : public ShpIOPort
{
	public:

		ShpInputBinary();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		void setValue(int8_t value);

	protected:

		int8_t m_pin;
		long m_timeout;
		const char *m_pinExpPortId;
		#ifdef SHP_GPIO_EXPANDER_I2C_H
		ShpGpioExpander *m_gpioExpander;
		#endif
		int8_t m_inputMode;
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
