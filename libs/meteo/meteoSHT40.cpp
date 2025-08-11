extern SHP_APP_CLASS *app;

ShpMeteoSHT40::ShpMeteoSHT40() :
																	m_address(-1),
																	m_bus(NULL),
																	m_sensor(NULL),
																	m_sensorStarted(false),
																	m_measureInterval(60000),
																	m_nextMeasure(0),
																	m_needSend(false),
																	m_temperature(0.0),
																	m_humidity(0.0)
{
}

void ShpMeteoSHT40::init(JsonVariant portCfg)
{
	m_address = 0x44;

	ShpIOPort::init(portCfg);

	m_topicTemperature = m_valueTopic + "temperature/" + app->m_deviceId + "/" + m_portId;
	m_topicHumidity = m_valueTopic + "humidity/" + app->m_deviceId + "/" + m_portId;

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

void ShpMeteoSHT40::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{
		m_sensor = new Adafruit_SHT4x();
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}


void ShpMeteoSHT40::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	if (!m_sensorStarted)
	{
		m_sensorStarted = m_sensor->begin(m_bus->wire());
		if (!m_sensorStarted)
		{
			log (shpllError, "SHT40 not started");
			m_nextMeasure = now + 3 * m_measureInterval;
			return;
		}
		m_sensor->setPrecision(SHT4X_HIGH_PRECISION);
  	m_sensor->setHeater(SHT4X_NO_HEATER);
	}

	sensors_event_t humidity, temp;
  m_sensor->getEvent(&humidity, &temp);
	m_temperature = temp.temperature;
	m_humidity = humidity.relative_humidity;

	static char b[16];

	sprintf(b, "%.1f", m_temperature);
	app->publish(b, m_topicTemperature.c_str());

	sprintf(b, "%.1f", m_humidity);
	app->publish(b, m_topicHumidity.c_str());

	m_nextMeasure = now + m_measureInterval;
}
