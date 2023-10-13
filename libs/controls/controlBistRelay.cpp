extern SHP_APP_CLASS *app;
extern int8_t g_pwmChannel;


ShpControlBistRelay::ShpControlBistRelay () :
																				m_pin1(-1),
																				m_pin1ExpPortId(NULL),
																				m_pin1GpioExpander(NULL),
																				m_pin2(-1),
																				m_pin2ExpPortId(NULL),
																				m_pin2GpioExpander(NULL),
																				m_stateOn (1),
																				m_stateOff (0),
																				m_stateCurrent (0),
																				m_stateSaved (0),
																				m_ledMode (BISTRELAY_LEDMODE_NONE),
																				m_pinLed (0),
																				m_ledBr(0),
																				m_pwmChannel(0),
																				m_queueRequests(0),
																				m_doEndPulseAfter(0),
																				m_saveStateAfter(0),
																				m_saveInterval(20000)
{
	for (int i = 0; i < BISTRELAY_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].state = 0;
		m_queue[i].duration = 0;
	}
}

void ShpControlBistRelay::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "controlBistRel",
		"portId": "uio-6-5",
		"pin1": 16,
		"pin2": 15
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin1
	if (portCfg.containsKey("pin1"))
		m_pin1 = portCfg["pin1"];

	if (portCfg["pin1_expPortId"] != nullptr)
		m_pin1ExpPortId = portCfg["pin1_expPortId"].as<const char*>();

	if (m_pin1 < 0)
		return;

	// -- pin2
	if (portCfg.containsKey("pin2"))
		m_pin2 = portCfg["pin2"];

	if (portCfg["pin2_expPortId"] != nullptr)
		m_pin2ExpPortId = portCfg["pin2_expPortId"].as<const char*>();

	if (m_pin2 < 0)
		return;

	if (portCfg.containsKey("ledMode"))
		m_ledMode = portCfg["ledMode"];

	if (portCfg.containsKey("pinLed"))
		m_pinLed = portCfg["pinLed"];

	if (portCfg.containsKey("ledBr"))
		m_ledBr = portCfg["ledBr"];

	//Serial.printf("BISTRELAY: ledMode: %d, pinLed: %d\n", m_ledMode, m_pinLed);

	m_valid = true;

	m_stateCurrent = 0;
}

void ShpControlBistRelay::init2()
{
	if (m_pin1ExpPortId)
	{
		m_pin1GpioExpander = (ShpGpioExpander*)app->ioPort(m_pin1ExpPortId);
	}
	else
	{
		pinMode(m_pin1, OUTPUT);
	}

	if (m_pin2ExpPortId)
	{
		m_pin2GpioExpander = (ShpGpioExpander*)app->ioPort(m_pin2ExpPortId);
	}
	else
	{
		pinMode(m_pin2, OUTPUT);
	}

	if (m_ledMode == BISTRELAY_LEDMODE_BIN)
	{
		pinMode(m_pinLed, OUTPUT);
	}
	else if (m_ledMode == BISTRELAY_LEDMODE_PWM)
	{
		m_pwmChannel = g_pwmChannel++;

		ledcSetup(m_pwmChannel, 5000, 8);
		ledcAttachPin(m_pinLed, m_pwmChannel);
	}

	endPulse();

	int32_t savedState = stateLoad();
	if (savedState == -1)
		addQueueItem(m_stateOff, 5);
	else
		addQueueItem(savedState, 5);
}

void ShpControlBistRelay::setState(uint8_t state)
{
	m_stateCurrent = state;
	m_saveStateAfter = millis() + m_saveInterval;

	if (state == 0)
	{ // OFF
		setPinState1(LOW);
		setPinState2(HIGH);

		if (m_ledMode == BISTRELAY_LEDMODE_BIN)
			digitalWrite(m_pinLed, LOW);
		else if (m_ledMode == BISTRELAY_LEDMODE_PWM)
			ledcWrite(m_pwmChannel, 0);

		app->setValue(m_portId, "0", m_sendMode);
	}
	else if (state == 1)
	{ // ON
		setPinState1(HIGH);
		setPinState2(LOW);

		if (m_ledMode == BISTRELAY_LEDMODE_BIN)
			digitalWrite(m_pinLed, HIGH);
		else if (m_ledMode == BISTRELAY_LEDMODE_PWM)
			ledcWrite(m_pwmChannel, m_ledBr);

		app->setValue(m_portId, "1", m_sendMode);
	}
	else
	{
		log(shpllError, "Invalid state `%d`", state);
		return;
	}
}

void ShpControlBistRelay::setPinState1(uint8_t value)
{
	if (m_pin1ExpPortId)
	{
		if (m_pin1GpioExpander)
		{
			m_pin1GpioExpander->setPinState(m_pin1, value);
			log(shpllDebug, "pin1: set gpioExp pin state %d", value);
		}
		else
		{
			log(shpllError, "pin1: Invalid GPIO expander");
		}
	}
	else
	{
		digitalWrite(m_pin1, value);
		log(shpllDebug, "pin1: set state %d", value);
	}
}

void ShpControlBistRelay::setPinState2(uint8_t value)
{
	if (m_pin2ExpPortId)
	{
		if (m_pin2GpioExpander)
		{
			m_pin2GpioExpander->setPinState(m_pin2, value);
			log(shpllDebug, "pin2: set gpioExp pin state %d", value);
		}
		else
		{
			log(shpllError, "pin2: Invalid GPIO expander");
		}
	}
	else
	{
		digitalWrite(m_pin2, value);
		log(shpllDebug, "pin2: set state %d", value);
	}
}

void ShpControlBistRelay::endPulse()
{
	setPinState1(LOW);
	setPinState2(LOW);
}

void ShpControlBistRelay::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	/* PAYLOADS:
	 * "0" -> OFF
	 * "1" -> ON
	 * "P<delay-msecs>": PUSH:   1 <delay-msecs> 0
	 * "U<delay-msecs>": UNPUSH: 0 <delay-msecs> 1
	 */

	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	int valid = 0;
	int8_t state = m_stateOn;

	if (strcmp(payloadStr.c_str(), "1") == 0)
	{
		state = m_stateOn;
		valid = 1;
	}
	else if (strcmp(payloadStr.c_str(), "0") == 0)
	{
		state = m_stateOff;
		valid = 1;
	}
	else if (strcmp(payloadStr.c_str(), "!") == 0)
	{
		state = (m_stateCurrent == m_stateOff) ? m_stateOn : m_stateOff;
		valid = 1;
	}
	else if (payloadStr.charAt(0) == 'P' || payloadStr.charAt(0) == 'U')
	{
		char *err;
		long duration = strtol(payloadStr.c_str() + 1, &err, 10);
		if (*err)
		{
			return;
		}
		else
		{
			if (payloadStr.charAt(0) == 'P')
			{
				addQueueItem(m_stateOn, 5);
				addQueueItem(m_stateOff, duration + 5);
			}
			else
			{
				addQueueItem(m_stateOff, 5);
				addQueueItem(m_stateOn, duration + 5);
			}

			return;
		}
	}

	if (!valid)
	{
		Serial.println("INVALID");
		return;
	}

	if (valid == 1)
	{
		Serial.printf("--ADD-ITEM1: %d\n", state);
		addQueueItem(state, 1);
	}
}

void ShpControlBistRelay::addQueueItem(int8_t state, unsigned long startAfter)
{
	for (int i = 0; i < BISTRELAY_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].state = state;
		m_queue[i].duration = 50;
		m_queue[i].startAfter = millis() + startAfter;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpControlBistRelay::runQueueItem(int i)
{
	m_queueRequests--;

	if (m_queue[i].duration)
	{
		m_doEndPulseAfter = millis() + m_queue[i].duration;
	}

	setState(m_queue[i].state);


	m_queue[i].qState = qsFree;
}

void ShpControlBistRelay::loop()
{
	if (m_doEndPulseAfter)
	{
		unsigned long now = millis();
		if (now > m_doEndPulseAfter)
		{
			endPulse();
			m_doEndPulseAfter = 0;
			return;
		}
	}

	unsigned long now = millis();

	if (m_queueRequests == 0)
	{
		if (m_saveStateAfter && m_stateSaved != m_stateCurrent && now > m_saveStateAfter)
		{
			stateSave(m_stateCurrent);
			m_stateSaved = m_stateCurrent;
			m_saveStateAfter = 0;
			Serial.println("----STATE---SAVED---");
		}
		return;
	}

	for (int i = 0; i < BISTRELAY_CONTROL_QUEUE_LEN; i++)
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

