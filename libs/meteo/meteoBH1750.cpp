extern SHP_APP_CLASS *app;

ShpMeteoBH1750::ShpMeteoBH1750() :
																	m_address(-1),
																	m_bus(NULL),
																	m_sensor(NULL),
																	m_measureInterval(10000),
																	m_nextMeasure(0),
																	m_sendTimeout(180000),
																	m_lastSendMillis(0),
																	m_lightLevel(0.0)
{
}

void ShpMeteoBH1750::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
			"type": "meteoBH1750",
			"portId": "uio-5-1",
			"i2cBusPortId": "i2c_1",
			"address": "23"
		}
	-----------------------------*/

	m_address = 0x23;

	ShpIOPort::init(portCfg);

	// -- busPortid
	m_busPortId = NULL;
	if (portCfg["i2cBusPortId"] != nullptr)
		m_busPortId = portCfg["i2cBusPortId"].as<const char*>();

	if (!m_busPortId)
		return;

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

void ShpMeteoBH1750::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{
		m_sensor = new BH1750();
		bool status = m_sensor->begin(BH1750::CONTINUOUS_HIGH_RES_MODE, m_address, m_bus->wire());
		if (!status)
			log (shpllError, "BH1750 not started");
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}

void ShpMeteoBH1750::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	if (m_sensor->measurementReady(true))
	{
		float thisLightLevel = m_sensor->readLightLevel();

		if (abs(thisLightLevel - m_lightLevel) > 1 || (now - m_lastSendMillis) > m_sendTimeout)
		{
			m_lightLevel = thisLightLevel;
			m_lastSendMillis = now;

			Serial.print("Light: ");
   		Serial.print(m_lightLevel);
    	Serial.println(" lx");

			m_nextMeasure = now + m_measureInterval;

			if (thisLightLevel < 0) {
				Serial.println(F("Error condition detected"));
				return;
			}

			static char b[16];
			sprintf(b, "%.1f", m_lightLevel);
			app->publish(b, m_valueTopic.c_str());

			if (thisLightLevel > 40000.0)
			{ // reduce measurement time - needed in direct sun light
				if (m_sensor->setMTreg(32)) {
					//Serial.println(F("Setting MTReg to low value for high light environment"));
				} else {
					Serial.println(F("Error setting MTReg to low value for high light environment"));
				}
				return;
      }

			if (thisLightLevel > 10.0)
			{ // typical light environment
				if (m_sensor->setMTreg(69)) {
					//Serial.println(F("Setting MTReg to default value for normal light environment"));
				} else {
					Serial.println(F("Error setting MTReg to default value for normal light environment"));
				}
				return;
      }

			if (thisLightLevel <= 10.0)
			{ // very low light environment
				if (m_sensor->setMTreg(138)) {
					//Serial.println(F("Setting MTReg to high value for low light environment"));
				} else {
					Serial.println(F("Error setting MTReg to high value for low light environment"));
				}
			}
		}
	}
}

