#ifndef SHP_METEO_BMP280_H
#define SHP_METEO_BMP280_H


class ShpMeteoBMP280 : public ShpIOPort
{
	public:

		ShpMeteoBMP280();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:

		int m_address;

		ShpBusI2C *m_bus;
		Adafruit_BMP280 *m_sensor;
		Adafruit_Sensor *m_sensor_temp;
		Adafruit_Sensor *m_sensor_pressure;

		bool m_sensorStarted;

		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		const char *m_busPortId;

		boolean m_needSend;
		float m_temperature;
		float m_pressure;

		String m_topicTemperature;
		String m_topicPressure;
};


#endif
