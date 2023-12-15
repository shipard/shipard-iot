#ifndef SHP_INPUT_CAPBTN_H
#define SHP_INPUT_CAPBTN_H


class ShpInputCapBtn : public ShpIOPort
{
	public:

		ShpInputCapBtn();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	protected:

		int8_t m_pin;
		int m_treshold;
		int m_ledStripPixel;
		int m_sendValue;
		unsigned long m_measureInterval; // ms
		unsigned long m_debounceDelay;
    unsigned long m_lastDebounceTime;
    bool m_touchState;
    bool m_lastTouchState;

		unsigned long m_nextMeasure;

    String m_portIdBuzzer;
    String m_portIdLedStrip;
		String m_ledStripPixelColor;
};


#endif
