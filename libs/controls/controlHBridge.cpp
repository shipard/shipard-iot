extern Application *app;


ShpControlHBridge::ShpControlHBridge () : 
																				m_pin1(-1),
																				m_pin1ExpPortId(NULL),
																				m_pin1GpioExpander(NULL),
																				m_pin2(-1),
																				m_pin2ExpPortId(NULL),
																				m_pin2GpioExpander(NULL),
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
		m_pin1ExpPortId = portCfg["pin1_expPortId"];

	if (m_pin1 < 0)
		return;

	// -- pin2
	if (portCfg.containsKey("pin2"))
		m_pin2 = portCfg["pin2"];

	if (portCfg["pin2_expPortId"] != nullptr)
		m_pin2ExpPortId = portCfg["pin2_expPortId"];

	if (m_pin2 < 0)
		return;

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

	setState(0);
}

void ShpControlHBridge::setState(uint8_t state)
{
	if (state == 0)
	{
		setPinState1(LOW);
		setPinState2(LOW);
	}
	else if (state == 1)
	{
		setPinState1(LOW);
		setPinState2(HIGH);
	}
	else if (state == 2)
	{
		setPinState1(HIGH);
		setPinState2(LOW);
	}
	else
	{
		log(shpllError, "Invalid state `%d`", state);
		return;
	}

	m_stateCurrent = state;
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

void ShpControlHBridge::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	/* PAYLOADS:
	 * "0" -> OFF
	 * "1" -> LEFT
	 * "2" -> RIGHT
	 * "<1|2>:<msecs>": pulse -> 2:1000 
	 */

	int valid = 0;
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

