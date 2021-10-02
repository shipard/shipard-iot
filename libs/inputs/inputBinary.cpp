extern Application *app;


ShpInputBinary::ShpInputBinary() : 
																	m_pin(-1),
																	m_timeout(100),
																	m_detectedValue(HIGH),
																	m_lastValue(127),
																	m_lastChangeMillis(0),
																	m_disable(false),
																	m_waitForChange(true),
																	m_needSend(false)
{
}

void ShpInputBinary::init(JsonVariant portCfg)
{
	/* inputBinary config format:
	 * ----------------------------
	{
		"type": "inputBinary",
		"portId": "uio-6-5",
		"pin": 16
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg["pin"] != nullptr)
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- timeout
	if (portCfg.containsKey("timeout"))
		m_timeout = portCfg["timeout"];

	if (!m_timeout)
		m_timeout = 100;

	pinMode(m_pin, INPUT);
	attachInterrupt(m_pin, std::bind(&ShpInputBinary::onPinChange, this, m_pin), RISING);
}

void ShpInputBinary::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
}

void ShpInputBinary::onPinChange(int pin)
{
	if (m_disable)
		return;
	m_needSend = true;
	m_disable = true;
	m_lastChangeMillis = millis();
}

void ShpInputBinary::loop()
{
	if (m_needSend)
	{
		app->publish((m_detectedValue == HIGH) ? "1" : "0", m_valueTopic.c_str());
		m_needSend = false;
		m_lastValue = m_detectedValue;
		m_waitForChange = true;
		return;
	}

	if (!m_waitForChange)
		return;

	unsigned long now = millis();
	if (now - m_lastChangeMillis < m_timeout)
		return;

	int newValue = digitalRead(m_pin);
	//Serial.printf("val=%d\n", newValue);
	if (newValue == m_lastValue)
		return;

	m_lastValue = newValue;
	m_disable = false;
	if (m_lastValue != m_detectedValue)
		m_waitForChange = false;

	char buffer[8];
	itoa(newValue, buffer, 10);
	app->publish(buffer, m_valueTopic.c_str());
}
