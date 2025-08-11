extern SHP_APP_CLASS *app;

ShpMeteoBMP280::ShpMeteoBMP280() :
																	m_address(-1),
																	m_bus(NULL),
																	m_sensor(NULL),
																	m_sensor_temp(NULL),
																	m_sensor_pressure(NULL),
																	m_sensorStarted(false),
																	m_measureInterval(60000),
																	m_nextMeasure(0),
																	m_needSend(false),
																	m_temperature(0.0),
																	m_pressure(0.0)
{
}

void ShpMeteoBMP280::init(JsonVariant portCfg)
{
	m_address = 0x77;

	ShpIOPort::init(portCfg);

	m_topicTemperature = m_valueTopic + "temperature/" + app->m_deviceId + "/" + m_portId;
	m_topicPressure = m_valueTopic + "pressure/" + app->m_deviceId + "/" + m_portId;

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

void ShpMeteoBMP280::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{
		m_sensor = new Adafruit_BMP280(m_bus->wire());
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}


void ShpMeteoBMP280::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	if (!m_sensorStarted)
	{
		m_sensorStarted = m_sensor->begin();
		if (!m_sensorStarted)
		{
			log (shpllError, "SHT40 not started");
			m_nextMeasure = now + 3 * m_measureInterval;
			return;
		}
		m_sensor_pressure = m_sensor->getPressureSensor();
		m_sensor_temp = m_sensor->getTemperatureSensor();
		m_sensor->setSampling(Adafruit_BMP280::MODE_NORMAL,
													 Adafruit_BMP280::SAMPLING_X2,
													 Adafruit_BMP280::SAMPLING_X16,
													 Adafruit_BMP280::FILTER_X16,
													 Adafruit_BMP280::STANDBY_MS_500);
	}

	sensors_event_t pressure, temp;
	m_sensor_temp->getEvent(&temp);
	m_sensor_pressure->getEvent(&pressure);
	m_temperature = temp.temperature;
	m_pressure = pressure.pressure;

	static char b[16];

	sprintf(b, "%.1f", m_temperature);
	app->publish(b, m_topicTemperature.c_str());

	sprintf(b, "%.1f", m_pressure);
	app->publish(b, m_topicPressure.c_str());

	m_nextMeasure = now + m_measureInterval;
}


