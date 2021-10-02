extern Application *app;


ShpInputAnalog::ShpInputAnalog() : 
																	m_pin(-1),
																	m_sendValue(iasvValue),
																	m_measureInterval(1000),
																	m_percentageMinValue(0),
																	m_percentageMaxValue(4095),
																	m_publishDelta(2),
																	m_nextMeasure(0),
																	m_lastValue(-1),
																	m_lastPercent(-1)
{
}

void ShpInputAnalog::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "inputAnalog",
		"portId": "uio-6-5",
		"sendValue": 0,
		"measureInterval": 1000,
		"publishDelta": 2,
		"pin": 16
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg["pin"] != nullptr)
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- sendValue
	if (portCfg["sendValue"] != nullptr)
		m_sendValue = portCfg["sendValue"];

	if (m_sendValue < 0 || m_sendValue > 1)	
		return;

	// -- measureInterval
	if (portCfg["measureInterval"] != nullptr)
		m_measureInterval = portCfg["measureInterval"];

	if (m_measureInterval < 0 || m_measureInterval > 900000)	
		m_measureInterval = 1000;

	// -- publishDelta	
	if (portCfg["publishDelta"] != nullptr)
		m_publishDelta = portCfg["publishDelta"];

	if (m_publishDelta < 0 || m_publishDelta > 100)	
		m_publishDelta = 2;

	pinMode(m_pin, INPUT);
}

void ShpInputAnalog::loop()
{
	ShpIOPort::loop();

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	int newValue = analogRead(m_pin);

	if (m_sendValue == iasvValue && newValue == m_lastValue)
		return;

	int correctedValue = newValue - m_percentageMinValue;
	int size = m_percentageMaxValue - m_percentageMinValue;
	int perc = long(correctedValue * 100) / size;

	if (m_sendValue == iasvPercent && perc == m_lastPercent)
		return;

	if (abs(perc - m_lastPercent) >= m_publishDelta)
	{
		char buffer[8];
		
		if (m_sendValue == iasvValue)
			itoa(newValue, buffer, 10);
		else if (m_sendValue == iasvPercent)
			itoa(perc, buffer, 10);

		if (!app->publish(buffer, m_valueTopic.c_str()))
			return;

		m_lastValue = newValue;
		m_lastPercent = perc;
	}
	
	m_nextMeasure = now + m_measureInterval;
}

