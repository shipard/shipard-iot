#ifndef SHP_POWER_BATT_USB_CHARGER_H
#define SHP_POWER_BATT_USB_CHARGER_H

#define SHP_USB_CHIP_BQ25896  1

class BQ25896;


class ShpPowerBattUSBCharger : public ShpIOPort
{
	public:

		ShpPowerBattUSBCharger();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

		void getValues();

	protected:

		void onActivityPin(int pin);


	private:

		int m_address;

		ShpBusI2C *m_bus;
    BQ25896 *m_sensor;


		int8_t m_activityPin;
		unsigned long m_measureInterval;
		volatile unsigned long m_nextMeasure;
		const char *m_busPortId;

		bool m_needSend;

		float m_VBattery;
		float m_VPowerSource;
		uint8_t m_VPowerSourceStatus;
		float m_ChargeCurrent;
		uint8_t m_powerSource;

};


#endif


