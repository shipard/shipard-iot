extern SHP_APP_CLASS *app;


ShpInputBinary::ShpInputBinary() :
																	m_pin(-1),
																	m_timeout(100),
																	m_pinExpPortId(NULL),
																	#ifdef SHP_GPIO_EXPANDER_I2C_H
																	m_gpioExpander(NULL),
																	#endif
																	m_inputMode(IBMODE_INT),
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
	if (portCfg.containsKey("pin"))
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- timeout
	if (portCfg.containsKey("timeout"))
		m_timeout = portCfg["timeout"];

	if (!m_timeout)
		m_timeout = 100;

	if (portCfg["pin_expPortId"] != nullptr)
		m_pinExpPortId = portCfg["pin_expPortId"].as<const char*>();
}

void ShpInputBinary::init2()
{
	#ifdef SHP_GPIO_EXPANDER_I2C_H
	if (m_pinExpPortId)
	{
		m_gpioExpander = (ShpGpioExpander*)app->ioPort(m_pinExpPortId);
		m_inputMode = IBMODE_GPIO_EXP;
		if (m_gpioExpander)
		{
			m_gpioExpander->setInputPinIOPort(m_pin, this);
			m_valid = true;
		}
	}
	else
	#endif
	{
		pinMode(m_pin, INPUT);
		attachInterrupt(m_pin, std::bind(&ShpInputBinary::onPinChange, this, m_pin), RISING);
		m_valid = true;
	}
}

void ShpInputBinary::onPinChange(int pin)
{
	if (m_disable)
		return;
	m_needSend = true;
	m_disable = true;
	m_lastChangeMillis = millis();
}

void ShpInputBinary::setValue(int8_t value)
{
	m_needSend = true;
	m_detectedValue = value;
}

void ShpInputBinary::loop()
{
	if (m_inputMode == IBMODE_INT)
	{
		if (m_needSend)
		{
			const char *pv = (m_detectedValue == HIGH) ? "1" : "0";
			if (m_sendAsAction)
				app->publishAction(m_portId, pv);

			app->publish(pv, m_valueTopic.c_str());

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

		const char *pv = (newValue == HIGH) ? "1" : "0";

		if (m_sendAsAction)
			app->publishAction(m_portId, pv);

		app->publish(pv, m_valueTopic.c_str());

		return;
	}

	if (m_needSend)
	{
		const char *pv = (m_detectedValue == HIGH) ? "1" : "0";
		m_needSend = false;

		if (m_sendAsAction)
			app->publishAction(m_portId, pv);

		app->publish(pv, m_valueTopic.c_str());
	}
}
