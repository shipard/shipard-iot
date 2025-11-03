#ifndef SHP_METEO_BME68X_H
#define SHP_METEO_BME68X_H


class ShpMeteoBME68x : public ShpIOPort
{
	public:

		ShpMeteoBME68x();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:

		int m_address;
		ShpBusI2C *m_bus;
		Adafruit_BME680 *m_sensor;
		bool m_sensorStarted;

		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		const char *m_busPortId;

		boolean m_needSend;
		float m_temperature;
		float m_humidity;
		float m_pressure;
		float m_gas;

		String m_topicTemperature;
		String m_topicHumidity;
		String m_topicPressure;
		String m_topicGas;
};


#endif
