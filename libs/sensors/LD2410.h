#ifndef SHP_SENSOR_LD2410_US_H
#define SHP_SENSOR_LD2410_US_H

#include <ld2410.h>

class ShpSensorLD2410 : public ShpIOPort
{
	public:

		ShpSensorLD2410();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

	private:

		int8_t m_pinRX;
		int8_t m_pinTX;

		unsigned long m_nextMeasure;

		ld2410 *m_radar;
		HardwareSerial *m_hwSerial;
};


#endif
