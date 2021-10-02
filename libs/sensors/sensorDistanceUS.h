#ifndef SHP_SENSOR_DISTANCE_US_H
#define SHP_SENSOR_DISTANCE_US_H



class ShpSensorDistanceUS : public ShpIOPort 
{
	public:
		ShpSensorDistanceUS();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	private:

		int8_t m_pinTrigger;
		int8_t m_pinEcho;
		unsigned long m_measureInterval;
		int m_cntShots;
		int m_shotInterval;
		int m_lastDistance;
		int m_sendDeltaFromLastDistance;
		unsigned long m_nextMeasure;

		NewPing *m_sensor;
		
};


#endif
