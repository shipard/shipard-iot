#ifndef SHP_DATA_ONE_WIRE_H
#define SHP_DATA_ONE_WIRE_H


class ShpDataOneWire : public ShpIOPort {
	public:

		ShpDataOneWire();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		void scan();
		void scanDevices();
		void readValues();

		void sensorAddressStr(DeviceAddress deviceAddress, char address[]);

	protected:

		struct oneSensorState
		{
			DeviceAddress deviceAddress;
			char deviceAddressStr[17];
			char topic[MQTT_MAX_TOPIC_LEN];

			float pastTemp;
			float newTemp;
		};


		OneWire *m_oneWire;
		DallasTemperature *m_sensors;
		int8_t m_pin;
		int m_readInterval;
		unsigned long m_lastRead;

		int m_countDevices;
		oneSensorState *m_sensorsStates;


};


#endif
