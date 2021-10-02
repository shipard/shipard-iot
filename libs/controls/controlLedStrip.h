#ifndef SHP_CONTROL_LEDSTRIP_H
#define SHP_CONTROL_LEDSTRIP_H


#define LEDSTRIP_COUNT_MODES 6
#define LEDSTRIP_INVALID_MODE_NAME 255


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
