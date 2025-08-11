#ifndef SHP_METEO_SHT40_H
#define SHP_METEO_SHT40_H


class ShpMeteoSHT40 : public ShpIOPort
{
	public:

		ShpMeteoSHT40();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:

		int m_address;

		ShpBusI2C *m_bus;
		Adafruit_SHT4x *m_sensor;
		bool m_sensorStarted;

		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		const char *m_busPortId;

		boolean m_needSend;
		float m_temperature;
		float m_humidity;

		String m_topicTemperature;
		String m_topicHumidity;
};


#endif
