extern SHP_APP_CLASS *app;

ShpMeteoBME68x::ShpMeteoBME68x() :
																	m_address(-1),
																	m_bus(NULL),
																	m_sensor(NULL),
																	m_sensorStarted(false),
																	m_measureInterval(60000),
																	m_nextMeasure(0),
																	m_needSend(false),
																	m_temperature(0.0),
																	m_humidity(0.0),
																	m_pressure(0.0),
																	m_gas(0.0)
{
}

void ShpMeteoBME68x::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
			"type": "meteoBME680",
			"portId": "uio-5-1",
			"i2cBusPortId": "i2c_1",
		}
	-----------------------------*/

	m_address = 0x77;

	ShpIOPort::init(portCfg);

	m_topicTemperature = m_valueTopic + "temperature/" + app->m_deviceId + "/" + m_portId;
	m_topicHumidity = m_valueTopic + "humidity/" + app->m_deviceId + "/" + m_portId;
	m_topicPressure = m_valueTopic + "pressure/" + app->m_deviceId + "/" + m_portId;
	m_topicGas = m_valueTopic + "gas/" + app->m_deviceId + "/" + m_portId;

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

void ShpMeteoBME68x::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{
		m_sensor = new Adafruit_BME680(m_bus->wire());
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}


void ShpMeteoBME68x::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	if (!m_sensorStarted)
	{
		m_sensorStarted = m_sensor->begin(m_address, true);
		if (!m_sensorStarted)
		{
			log (shpllError, "BME68x not started");
			m_nextMeasure = now + 3 * m_measureInterval;
			return;
		}
		m_sensor->setTemperatureOversampling(BME680_OS_8X);
		m_sensor->setHumidityOversampling(BME680_OS_2X);
		m_sensor->setPressureOversampling(BME680_OS_4X);
		m_sensor->setIIRFilterSize(BME680_FILTER_SIZE_3);
		m_sensor->setGasHeater(320, 150); // 320*C for 150 ms
	}

	m_sensor->performReading();

	m_temperature = m_sensor->temperature;
	m_humidity = m_sensor->humidity;
	m_pressure = m_sensor->pressure / 100.0;
	m_gas = m_sensor->gas_resistance / 1000.0;

	static char b[16];

	sprintf(b, "%.1f", m_temperature);
	app->publish(b, m_topicTemperature.c_str());

	sprintf(b, "%.1f", m_humidity);
	app->publish(b, m_topicHumidity.c_str());

	sprintf(b, "%.1f", m_pressure);
	app->publish(b, m_topicPressure.c_str());

	sprintf(b, "%.1f", m_gas);
	app->publish(b, m_topicGas.c_str());

	Serial.printf("BME68x (%ld):\n", millis());
	Serial.println(m_temperature);
	Serial.println(m_humidity);
	Serial.println(m_pressure);
	Serial.println(m_gas);


	m_nextMeasure = now + m_measureInterval;
}
