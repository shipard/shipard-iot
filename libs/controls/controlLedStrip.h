#ifndef SHP_CONTROL_LEDSTRIP_H
#define SHP_CONTROL_LEDSTRIP_H


#define LEDSTRIP_COUNT_MODES 29
#define LEDSTRIP_INVALID_MODE_NAME 255
#define LEDSTRIP_SET_BRIGHTNESS 254
#define LEDSTRIP_SET_SPEED 253

// -- shipard color mode cfg item --> board value
/* "enumValues": {
		"0": "G-R-B (WS2812, Neopixel,...)", 
		"1": "R-B-G (WS2811, SM16703,...)",
		"2": "R-G-B",
		"3": "G-B-R",
		"4": "B-R-G",
		"5": "B-G-R"
	}
*/
#define LED_STRIP_COLOR_MODES_CNT 6
static unsigned long LED_STRIP_COLOR_MODES[LED_STRIP_COLOR_MODES_CNT] = {NEO_GRB, NEO_RBG, NEO_RGB, NEO_GBR, NEO_BRG, NEO_BGR};


class ShpControlLedStrip : public ShpIOPort 
{
	public:

		ShpControlLedStrip();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

	
	protected:

		void addQueueItemFromMessage(const char* topic, byte* payload, unsigned int length);

		void setColorAll (uint32_t color);

	protected:

		int8_t m_pin;
		int m_cntLeds;
		int8_t m_colorMode;

		struct ModeSetting
		{
			int8_t qState;
			unsigned long startAfter;

			int8_t itemType;
			int8_t pinNumber;
			int8_t pinState;
		};

		struct ModeNames
		{
			const char *name;
			uint8_t mode;
		};

		Adafruit_NeoPixel *m_strip;
		WS2812FX *m_ws2812fx;


		uint8_t modeFromString(const char *mode);

};


#endif
