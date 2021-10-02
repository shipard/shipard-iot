#ifndef SHP_METEO_BME280_H
#define SHP_METEO_BME280_H


class ShpMeteoBME280 : public ShpIOPort 
{
	public:
		
		ShpMeteoBME280();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:

		int m_address;

		ShpBusI2C *m_bus;
		Adafruit_BME280 *m_sensor;

		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		const char *m_busPortId;

		boolean m_needSend;
		float m_temperature;
		float m_humidity;
		float m_pressure;

		String m_topicTemperature;
		String m_topicHumidity;
		String m_topicPressure;
};


#endif
