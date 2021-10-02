extern Application *app;

ShpMeteoBME280::ShpMeteoBME280() : 
																	m_address(-1),
																	m_bus(NULL),
																	m_sensor(NULL),
																	m_measureInterval(60000),
																	m_nextMeasure(0),
																	m_needSend(false),
																	m_temperature(0.0),
																	m_humidity(0.0),
																	m_pressure(0.0)
{
}

void ShpMeteoBME280::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
			"type": "meteoBME280",
			"portId": "uio-5-1",
			"i2cBusPortId": "i2c_1",
			"address": "76"
		}
	-----------------------------*/

	m_address = 0x76;

	ShpIOPort::init(portCfg);

	m_topicTemperature = m_valueTopic + "temperature/" + app->m_deviceId + "/" + m_portId;
	m_topicHumidity = m_valueTopic + "humidity/" + app->m_deviceId + "/" + m_portId;
	m_topicPressure = m_valueTopic + "pressure/" + app->m_deviceId + "/" + m_portId;

	// -- busPortid
	m_busPortId = NULL;
	if (portCfg["i2cBusPortId"] != nullptr)
		m_busPortId = portCfg["i2cBusPortId"];

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

void ShpMeteoBME280::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{		
		m_sensor = new Adafruit_BME280();
		bool status = m_sensor->begin(m_address, m_bus->wire());
		if (!status)
			log (shpllError, "BME280 not started");
		/*
		Serial.println ("BME INIT");
		Serial.println (m_address);
		Serial.println (status);
		*/
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}


void ShpMeteoBME280::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	m_temperature = m_sensor->readTemperature();
	m_humidity = m_sensor->readHumidity();
	m_pressure = m_sensor->readPressure() / 100 + 32;

	static char b[16];

	sprintf(b, "%.1f", m_temperature);
	app->publish(b, m_topicTemperature.c_str());

	sprintf(b, "%.1f", m_humidity);
	app->publish(b, m_topicHumidity.c_str());

	sprintf(b, "%.1f", m_pressure);
	app->publish(b, m_topicPressure.c_str());

	/*
	Serial.println("BME280:");
	Serial.println(m_temperature);
	Serial.println(m_humidity);
	Serial.println(m_pressure);
	*/

	m_nextMeasure = now + m_measureInterval;
}
