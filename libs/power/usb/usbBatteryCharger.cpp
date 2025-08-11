extern SHP_APP_CLASS *app;

#include "BQ25896.h";




ShpPowerBattUSBCharger::ShpPowerBattUSBCharger() :
                                                    m_address(-1),
                                                    m_bus(NULL),
                                                    m_sensor(NULL),
																										m_activityPin(-1),
                                                    m_measureInterval(60000),
                                                    m_nextMeasure(2000),
                                                    m_needSend(true),
																										m_VBattery (0.0f),
																										m_VPowerSource (0.0f),
																										m_VPowerSourceStatus (126),
																										m_powerSource(0),
																										m_ChargeCurrent (0.0f)
{
}

void ShpPowerBattUSBCharger::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
			"type": "power/battusb",
			"portId": "battery",
			"i2cBusPortId": "i2c",
			"address": "6a"
		}
	-----------------------------*/

	m_activityPin = GPIO_NUM_21;

	m_address = 0x6a;
	Serial.println("ShpPowerBattUSBCharger::init");
	ShpIOPort::init(portCfg);

	// -- busPortid
	m_busPortId = "i2c";
	if (portCfg["i2cBusPortId"] != nullptr)
		m_busPortId = portCfg["i2cBusPortId"].as<const char*>();

	// -- address
	if (portCfg["address"] != nullptr && portCfg["address"] != "")
	{
		char *err;
		m_address = strtol(portCfg["address"], &err, 16);
		if (*err)
		{
			log (shpllError, "Invalid I2C address format");
			return;
		}

	}
	if (m_address < 0 || m_address > 127)
	{
		log (shpllError, "Invalid I2C address number");
		return;
	}

	m_valid = true;
}

void ShpPowerBattUSBCharger::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{
	  m_sensor = new BQ25896 (*m_bus->wire());
    m_sensor->begin();
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}

	if (m_sensor)
	{
		delay(100);
		//m_sensor->properties();
		m_sensor->setCharge_Voltage_Limit(4.2);
		//m_sensor->setPreCharge_Current_Limit(1);
		m_sensor->setFast_Charge_Current_Limit(3.0);

		if (m_activityPin >= 0)
		{
			pinMode(m_activityPin, INPUT);
			attachInterrupt(m_activityPin, std::bind(&ShpPowerBattUSBCharger::onActivityPin, this, m_activityPin), FALLING);
		}
	}
}

void ShpPowerBattUSBCharger::onActivityPin(int pin)
{
	m_nextMeasure = millis() + 1000;
}

void ShpPowerBattUSBCharger::getValues()
{
	if (!m_sensor)
	{
		log (shpllError, "PWR IC NOT INITIALIZED");
		return;
	}

	float prev_VPowerSource	= m_VPowerSource;
	float prev_VBattery = m_VBattery;
	float prev_ChargeCurrent = m_ChargeCurrent;
	uint8_t prev_VPowerSourceStatus = m_VPowerSourceStatus;

	m_sensor->properties();

	m_VPowerSource = m_sensor->getVBUS();
	m_VBattery = m_sensor->getVBAT();
	m_ChargeCurrent = m_sensor->getICHG();

	/*
	            enum class VBUS_STAT
            {
                NO_INPUT        = 0,
                USB_HOST        = 1,
                ADAPTER         = 2,
                OTG             = 7
            };
	*/
	m_powerSource = (uint8_t)m_sensor->getVBUS_STATUS();

	/*
		NOT_CHARGING    = 0,
		PRE_CHARGE      = 1,
		FAST_CHARGE     = 2,
		CHARGE_DONE     = 3
	*/
	m_VPowerSourceStatus = (uint8_t)m_sensor->getCHG_STATUS();

	if ((prev_VPowerSourceStatus != m_VPowerSourceStatus) || (prev_VPowerSource!= m_VPowerSource) || (prev_VBattery != m_VBattery) || (prev_ChargeCurrent != m_ChargeCurrent))
		m_needSend = true;

	/*
	Serial.println("Battery Management System Parameter : \n===============================================================");
  Serial.print("VBUS : "); Serial.println(m_sensor->getVBUS());
  Serial.print("VSYS : "); Serial.println(m_sensor->getVSYS());
  Serial.print("VBAT : "); Serial.println(m_sensor->getVBAT());
  Serial.print("ICHG : "); Serial.println(m_sensor->getICHG(),4);
  Serial.print("TSPCT : "); Serial.println(m_sensor->getTSPCT());
  Serial.print("Temperature : "); Serial.println(m_sensor->getTemperature());

	Serial.print("Charge_Voltage_Limit : "); Serial.println(m_sensor->getCharge_Voltage_Limit());

  Serial.print("FS_Current Limit : "); Serial.println(m_sensor->getFast_Charge_Current_Limit());
  Serial.print("IN_Current Limit : "); Serial.println(m_sensor->getInput_Current_Limit());
  Serial.print("PRE_CHG_Current Limit : "); Serial.println(m_sensor->getPreCharge_Current_Limit());
  Serial.print("TERM_Current Limit : "); Serial.println(m_sensor->getTermination_Current_Limit());

  Serial.print("Charging Status : "); Serial.println(m_sensor->getCHG_STATUS()==BQ25896::CHG_STAT::NOT_CHARGING?" not charging":
                                                          (m_sensor->getCHG_STATUS()==BQ25896::CHG_STAT::PRE_CHARGE ?" pre charging":
                                                          (m_sensor->getCHG_STATUS()==BQ25896::CHG_STAT::FAST_CHARGE?" Fast charging":"charging done")));

  Serial.print("VBUS Status : "); Serial.println(m_sensor->getVBUS_STATUS()==BQ25896::VBUS_STAT::NO_INPUT?" not input":
                                                          (m_sensor->getVBUS_STATUS()==BQ25896::VBUS_STAT::USB_HOST ?" USB host":
                                                          (m_sensor->getVBUS_STATUS()==BQ25896::VBUS_STAT::ADAPTER?" Adapter":"OTG")));





 Serial.print("VSYS Status : "); Serial.println(m_sensor->getVSYS_STATUS()==BQ25896::VSYS_STAT::IN_VSYSMIN?" In VSYSMIN regulation (BAT < VSYSMIN)":
                                                      "Not in VSYSMIN regulation (BAT > VSYSMIN)");

  Serial.print("Temperature rank : "); Serial.println(m_sensor->getTemp_Rank()==BQ25896::TS_RANK::NORMAL?" Normal":
                                                          (m_sensor->getTemp_Rank()==BQ25896::TS_RANK::WARM ?" Warm":
                                                          (m_sensor->getTemp_Rank()==BQ25896::TS_RANK::COOL?" Cool":
                                                          (m_sensor->getTemp_Rank()==BQ25896::TS_RANK::COLD?" Cold":"HOT"))));

  Serial.print("Charger fault status  : "); Serial.println(m_sensor->getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::NORMAL?" Normal":
                                                          (m_sensor->getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::INPUT_FAULT ?" Input Fault":
                                                          (m_sensor->getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::THERMAL_SHUTDOWN?" Thermal Shutdown":"TIMER_EXPIRED")));
	*/

}

void ShpPowerBattUSBCharger::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

  if (!app->m_serverConnected)
    return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	getValues();
	m_nextMeasure = now + m_measureInterval;

	//if (!m_needSend)
	//	return;

	Serial.println("USB/BATT - publish data:");

	float minBatteryVoltage = 3.2;
	int bp = 0;
	if (m_VBattery > minBatteryVoltage)
		bp = int(((m_VBattery - minBatteryVoltage) / (4.3 - minBatteryVoltage)) * 100);

	app->setValue("pwr-batt-voltage", m_VBattery, SM_LOOP);
	app->setValue("pwr-batt-perc", bp, SM_LOOP);
	app->setValue("pwr-src", m_powerSource, SM_LOOP);
	app->setValue("pwr-src-status", m_VPowerSourceStatus, SM_LOOP);
	app->setValue("pwr-src-voltage", m_VPowerSource, SM_LOOP);
	app->setValue("pwr-charge-current", m_ChargeCurrent, SM_LOOP);

	if (m_powerSource)
		app->setChargingState(true);
	else
		app->setChargingState(false);

	m_needSend = false;
}
