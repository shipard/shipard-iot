extern SHP_APP_CLASS *app;


int8_t g_pwmChannel = 0;


ShpControlLevel::ShpControlLevel() :
																			m_pin(-1),
																			m_defaultValue(0),
																			m_startMax(0),
																			m_queueRequests(0)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].pinNumber = 0;
		m_queue[i].startAfter = 0;
		m_queue[i].pinValue = 0;
	}
}

void ShpControlLevel::init(JsonVariant portCfg)
{
	/* controlLevel config format:
	 * ----------------------------
	{
		"type": "controlBinary",
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

	m_pinValueMin = 0;
	m_pinValueMax = 255;
	m_pinValueCurrent = m_pinValueMin;
	m_pinValueLast = m_pinValueCurrent;
	m_steps = 30;

	if (portCfg.containsKey("defaultValue"))
	{
		int perc = portCfg["defaultValue"];
		m_defaultValue = (((m_pinValueMax - m_pinValueMin) / 100.0) * perc) + m_pinValueMin;
	}
	if (portCfg.containsKey("startMax"))
		m_startMax = portCfg["startMax"];

	m_pwmChannel = g_pwmChannel++;

	ledcSetup(m_pwmChannel, 25000, 8);
	ledcAttachPin(m_pin, m_pwmChannel);

	if (m_defaultValue)
	{
		if (m_startMax)
		{
			addQueueItem(m_pin, m_pinValueMax, 100);
			addQueueItem(m_pin, m_defaultValue, 3500);
		}
		else
			addQueueItem(m_pin, m_defaultValue, 100);
	}
}

void ShpControlLevel::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	addQueueItemFromMessage(payload, length);
}

void ShpControlLevel::addQueueItemFromMessage(byte* payload, unsigned int length)
{
	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	if (payloadStr == "!")
	{ // ON / OFF
		if (m_pinValueCurrent == m_pinValueMin)
			m_pinValueCurrent = m_pinValueLast;
		else
			m_pinValueCurrent = m_pinValueMin;

		ledcWrite(m_pwmChannel, m_pinValueCurrent);

		return;
	}

	boolean valid = false;

	uint32_t pinValue = 0;

	char* p;
	long convertedValue = strtol(payloadStr.c_str(), &p, 10);
	if (*p)
	{
		return;
	}
	else
	{
		//Serial.printf("CONVERTED VALUE: %d", convertedValue);
		if (convertedValue >= 1 && convertedValue <= m_steps)
		{
			if (convertedValue == 1)
				pinValue = m_pinValueMin;
			else if (convertedValue == m_steps)
				pinValue = m_pinValueMax;
			else
				pinValue = ((m_pinValueMax - m_pinValueMin)	/ (m_steps - 1)) * (convertedValue - 1);

			valid = true;
		}
	}

	if (!valid)
	{
		return;
	}

	addQueueItem(m_pin, pinValue, 10);
}

void ShpControlLevel::addQueueItem(int8_t pinNumber, uint32_t pinValue, unsigned long startAfter)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].pinNumber = pinNumber;
		m_queue[i].pinValue = pinValue;
		m_queue[i].startAfter = millis() + startAfter;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpControlLevel::runQueueItem(int i)
{
	m_queueRequests--;

	ledcWrite(m_pwmChannel, m_queue[i].pinValue);
	m_pinValueCurrent = m_queue[i].pinValue;
	m_pinValueLast = m_pinValueCurrent;

	m_queue[i].qState = qsFree;

	app->setValue(m_portId, (int)m_queue[i].pinValue, m_sendMode);
}

void ShpControlLevel::loop()
{
	if (m_queueRequests == 0)
		return;

	unsigned long now = millis();

	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsDoIt)
			continue;

		if (m_queue[i].startAfter > now)
			continue;

		m_queue[i].qState = qsRunning;
		runQueueItem(i);

		break;
	}
}

