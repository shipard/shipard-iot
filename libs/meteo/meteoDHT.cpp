extern Application *app;



ShpMeteoDHT::ShpMeteoDHT() : 
														m_pinData(-1), 
														m_measureInterval(60000),
														m_nextMeasure(0),
														m_needSend(false),
														m_temperature(0.0),
														m_humidity(0.0),
														m_sensor(nullptr)
{
}

void ShpMeteoDHT::init(JsonVariant portCfg)
{
	/* DHT22 temperature / humidity sensor:
	 * -----------------------------------------
	 {
			"type": "22",
			"portId": "uio-5-1",
			"pinData": 16
		}
	---------------------------------------------*/

	ShpIOPort::init(portCfg);

	// -- data pin
	if (portCfg["pinData"] != nullptr)
		m_pinData = portCfg["pinData"];

	if (m_pinData < 0)
		return;

	m_topicTemperature = m_valueTopic + "temperature/" + m_portId;
	m_topicHumidity = m_valueTopic + "humidity/" + m_portId;

	m_sensor = new DHT22();
	m_sensor->setup(m_pinData);

	m_sensor->setCallback(std::bind(&ShpMeteoDHT::onData, this, m_pinData));
}

void ShpMeteoDHT::onData(int8_t result)
{
	if (m_sensor->available() < 1)
		return;
	
	m_temperature = m_sensor->getTemperature();
	m_humidity = m_sensor->getHumidity();

	//Serial.printf("Temp: %.1f°C\nHumid: %.1f%%\n", m_temperature, m_humidity);

	m_needSend = true;
}

void ShpMeteoDHT::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	if (m_needSend)
	{
		char b[10];

		sprintf(b, "%.1f", m_temperature);
		app->publish(b, m_topicTemperature.c_str());

		sprintf(b, "%.1f", m_humidity);
		app->publish(b, m_topicHumidity.c_str());

		m_needSend = false;
		return;
	}

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	//Serial.println("DHT READ...");
	m_sensor->read();	

	m_nextMeasure = now + m_measureInterval;
}

