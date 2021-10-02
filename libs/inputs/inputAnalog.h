#ifndef SHP_INPUT_ANALOG_H
#define SHP_INPUT_ANALOG_H

#define iasvValue 0
#define iasvPercent 1

class ShpInputAnalog : public ShpIOPort 
{
	public:

		ShpInputAnalog();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	protected:

		int8_t m_pin;
		int m_sendValue;
		long m_measureInterval; // ms
		int m_percentageMinValue;
		int m_percentageMaxValue;
		int m_publishDelta;

		unsigned long m_nextMeasure;
		int m_lastValue;
		int m_lastPercent;

};


#endif
