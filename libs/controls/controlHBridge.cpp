extern SHP_APP_CLASS *app;
extern int8_t g_pwmChannel;


ShpControlHBridge::ShpControlHBridge () :
																				m_pin1(-1),
																				m_pin1ExpPortId(NULL),
																				m_pin1GpioExpander(NULL),
																				m_pin2(-1),
																				m_pin2ExpPortId(NULL),
																				m_pin2GpioExpander(NULL),
																				m_ledMode (HBRIDGE_LEDMODE_NONE),
																				m_pinLed (0),
																				m_ledBr(0),
																				m_pwmChannel(0),
																				m_queueRequests(0),
																				m_switchToState(0),
																				m_switchToStateAfter(0)
{
	for (int i = 0; i < HBRIDGE_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].state = 0;
		m_queue[i].duration = 0;
	}
}

void ShpControlHBridge::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "controlHBridge",
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

	//Serial.printf("HBRIDGE: ledMode: %d, pinLed: %d\n", m_ledMode, m_pinLed);

	m_valid = true;

	m_stateCurrent = 0;
}

void ShpControlHBridge::init2()
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

	if (m_ledMode == HBRIDGE_LEDMODE_BIN)
	{
		pinMode(m_pinLed, OUTPUT);
	}
	else if (m_ledMode == HBRIDGE_LEDMODE_PWM)
	{
		m_pwmChannel = g_pwmChannel++;

		ledcSetup(m_pwmChannel, 5000, 8);
		ledcAttachPin(m_pinLed, m_pwmChannel);
	}

	setState(0);
}

void ShpControlHBridge::setState(uint8_t state)
{
	if (state == 0)
	{
		setPinState1(LOW);
		setPinState2(LOW);
		app->setValue(m_portId, "0", m_sendMode);
	}
	else if (state == 1)
	{
		setPinState1(LOW);
		setPinState2(HIGH);

		if (m_ledMode == HBRIDGE_LEDMODE_BIN)
			digitalWrite(m_pinLed, LOW);
		else if (m_ledMode == HBRIDGE_LEDMODE_PWM)
			ledcWrite(m_pwmChannel, 0);

		app->setValue(m_portId, "1", m_sendMode);
	}
	else if (state == 2)
	{
		setPinState1(HIGH);
		setPinState2(LOW);

		if (m_ledMode == HBRIDGE_LEDMODE_BIN)
			digitalWrite(m_pinLed, HIGH);
		else if (m_ledMode == HBRIDGE_LEDMODE_PWM)
			ledcWrite(m_pwmChannel, m_ledBr);

		app->setValue(m_portId, "2", m_sendMode);
	}
	else
	{
		log(shpllError, "Invalid state `%d`", state);
		return;
	}

	m_stateCurrent = state;

	//	app->setValue(m_portId, state, m_sendMode);

}

void ShpControlHBridge::setPinState1(uint8_t value)
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

void ShpControlHBridge::setPinState2(uint8_t value)
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

void ShpControlHBridge::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	/* PAYLOADS:
	 * "0" -> OFF
	 * "1" -> LEFT / OFF
	 * "2" -> RIGHT / ON
	 * "<1|2>:<msecs>": pulse -> 2:1000
	 * "P:<delay-msecs>": PUSH:   2:50 <delay-msecs> 1:50
	 * "U:<delay-msecs>": UNPUSH: 1:50 <delay-msecs> 2:50
	 */

	int valid = 0;

	if (payload[0] == 'P' || payload[0] == 'U')
	{
		Serial.println("push / unpush");
		long stateLen = 0;
		valid = 1;
		if (length > 2 && payload[1] == ':')
		{
			char number[10];
			strncpy(number, (char*)payload+2, length - 2);
			number [length - 2] = 0;
			char *err;
			stateLen = strtol(number, &err, 10);
			if (*err)
				valid = 0;
		}

		if (valid)
		{
			if (payload[0] == 'P') // PUSH
			{
				addQueueItem(2, 50, 1);
				addQueueItem(1, 50, stateLen + 1);
			}
			else
			if (payload[0] == 'U') // UNPUSH
			{
				addQueueItem(1, 50, 1);
				addQueueItem(2, 50, stateLen + 1);
			}
		}

		return;
	}

	int8_t state = -1;
	long duration = 0;

	if (length < 10)
	{
		state = (uint8_t)payload[0] - 48;
		if (state >= 0 && state <= 2)
			valid = 1;

		if (length > 2 && payload[1] == ':')
		{
			char number[10];
			strncpy(number, (char*)payload+2, length - 2);
			number [length - 2] = 0;
			char *err;
			duration = strtol(number, &err, 10);
			if (*err)
				valid = 0;
		}
	}

	if (!valid)
	{
		Serial.println("INVALID");
		return;
	}

	if (valid == 1)
	{
		addQueueItem(state, duration, 1);
	}
}

void ShpControlHBridge::addQueueItem(int8_t state, long duration, unsigned long startAfter)
{
	for (int i = 0; i < HBRIDGE_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].state = state;
		m_queue[i].duration = duration;
		m_queue[i].startAfter = millis() + startAfter;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpControlHBridge::runQueueItem(int i)
{
	m_queueRequests--;

	if (m_queue[i].duration)
	{
		m_switchToState = 0;
		m_switchToStateAfter = millis() + m_queue[i].duration;
	}

	setState(m_queue[i].state);


	m_queue[i].qState = qsFree;
}

void ShpControlHBridge::loop()
{
	if (m_switchToStateAfter)
	{
		unsigned long now = millis();
		if (now > m_switchToStateAfter)
		{
			setState(m_switchToState);
			m_switchToStateAfter = 0;
			m_switchToState = 0;
			return;
		}
	}

	if (m_queueRequests == 0)
		return;

	unsigned long now = millis();

	for (int i = 0; i < HBRIDGE_CONTROL_QUEUE_LEN; i++)
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

