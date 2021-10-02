#ifndef SHP_METEO_BH1750_H
#define SHP_METEO_BH1750_H


class ShpMeteoBH1750 : public ShpIOPort 
{
	public:
		
		ShpMeteoBH1750();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

	private:

		int m_address;

		ShpBusI2C *m_bus;
		BH1750 *m_sensor;

		unsigned long m_measureInterval;
		unsigned long m_nextMeasure;
		const char *m_busPortId;

		unsigned long m_sendTimeout;
		unsigned long m_lastSendMillis;
		float m_lightLevel;
	};


#endif
