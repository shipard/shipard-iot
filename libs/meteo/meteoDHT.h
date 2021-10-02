#ifndef SHP_METEO_DHT_US_H
#define SHP_METEO_DHT_US_H



class ShpMeteoDHT : public ShpIOPort 
{
	public:
		ShpMeteoDHT();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	private:

		int8_t m_pinData;
		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		boolean m_needSend;
		float m_temperature;
		float m_humidity;

		DHT22 *m_sensor;

		void onData(int8_t result);

		String m_topicTemperature;
		String m_topicHumidity;
		
};


#endif
