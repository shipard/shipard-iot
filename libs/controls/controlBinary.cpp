extern SHP_APP_CLASS *app;


ShpControlBinary::ShpControlBinary() : 
																				m_pin(-1),
																				m_reverse(false),
																				m_pinExpPortId(NULL),
																				#ifdef SHP_GPIO_EXPANDER_I2C_H
																				m_gpioExpander(NULL),
																				#endif
																				m_queueRequests(0)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].qType = qbcitSetValue;
		m_queue[i].startAfter = 0;
		m_queue[i].pinState = 0;
	}

	m_sendMode = SM_LOOP;
}

void ShpControlBinary::init(JsonVariant portCfg)
{
	/* controlBinary config format:
	 * ----------------------------
	{
		"type": "controlBinary",
		"portId": "uio-6-5",
		"reverse": 1,
		"pin": 16
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg.containsKey("pin"))
		m_pin = portCfg["pin"];

	if (m_pin < 0)
	{
		return;
	}

	if (portCfg["reverse"] != nullptr)
		m_reverse = portCfg["reverse"];

	if (m_reverse)
	{
		m_PinStateOn = LOW;
		m_PinStateOff = HIGH;
	}
	else
	{
		m_PinStateOn = HIGH;
		m_PinStateOff = LOW;
	}


	m_PinStateCurrent = m_PinStateOff;
	if (portCfg.containsKey("onAfterStart") && portCfg["onAfterStart"] == 1)
		m_PinStateCurrent = m_PinStateOn;

	if (portCfg["pin_expPortId"] != nullptr)
		m_pinExpPortId = portCfg["pin_expPortId"];
}

void ShpControlBinary::init2()
{
	#ifdef SHP_GPIO_EXPANDER_I2C_H
	if (m_pinExpPortId)
	{
		m_gpioExpander = (ShpGpioExpander*)app->ioPort(m_pinExpPortId);
	}
	else
	#endif
	{
		pinMode(m_pin, OUTPUT);
	}

	setPinState(m_PinStateCurrent);
}

void ShpControlBinary::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	addQueueItemFromMessage(topic, payload, length);
}

void ShpControlBinary::addQueueItemFromMessage(const char* topic, byte* payload, unsigned int length)
{
	/* PAYLOADS:
	 * "0" -> OFF
	 * "1" -> ON
	 * "!" -> switch current value
	 * "P<msecs>": press button -> P1000 
	 * "F[D<msecs>]|[R<repeatCount>]:<durationOn:DurationOff,...>": flash -> 100:300
	 */

	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	int valid = 0;
	int8_t pinState = m_PinStateOn;

	if (strcmp(payloadStr.c_str(), "1") == 0)
	{
		pinState = m_PinStateOn;
		valid = 1;
	}
	else if (strcmp(payloadStr.c_str(), "0") == 0)
	{
		pinState = m_PinStateOff;
		valid = 1;
	}
	else if (strcmp(payloadStr.c_str(), "!") == 0)
	{
		pinState = (m_PinStateCurrent == m_PinStateOff) ? m_PinStateOn : m_PinStateOff;
		valid = 1;
	}
	else if (payloadStr.charAt(0) == 'P')
	{
		char *err;
		long duration = strtol(payloadStr.c_str() + 1, &err, 10);
		if (*err) 
		{
			return;
		}
		else
		{
			addQueueItemPressButton(duration, 1);
			return;
		}
	}
	else if (payloadStr.charAt(0) == 'F')
	{
		int totalDuration = 0;
		int repeatCnt = 0;
		int scenarioDurations[BINARY_CONTROL_SCENARIO_LEN];
		int scenarioLen = 0;

		char *oneItem = NULL;
		int paramNdx = 0;
		
  	oneItem = strtok(payloadStr.begin(), ":");
		while (oneItem != NULL && paramNdx < BINARY_CONTROL_SCENARIO_LEN)
		{
			if (paramNdx == 0)
			{
				if (oneItem[1] != 0)
				{
					char *err;
					long number = strtol(oneItem + 2, &err, 10);
					if (*err) 
					{

						return;	
					}
					else
					{
						if (payloadStr.charAt(1) == 'R')
							repeatCnt = number;
						else if (payloadStr.charAt(1) == 'D')
						{
							totalDuration = number;
						}	
						else
						{
							return;						
						}
					}
				}
			}
			else
			{
				char *err;
				long number = strtol(oneItem, &err, 10);
				if (*err) 
				{

					return;	
				}
				else
				{
					scenarioDurations[scenarioLen++] = number;
				}
			}

			oneItem = strtok (NULL, ":");

			paramNdx++;
		}

		addQueueItemFlash(scenarioDurations, scenarioLen, repeatCnt, totalDuration, 1);
		return;
	}

	if (!valid)
	{
		Serial.println("INVALID");
		return;
	}

	if (valid == 1)
	{
		addQueueItem(pinState, 1);
	}
}

void ShpControlBinary::addQueueItem(int8_t pinState, unsigned long startAfter)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].qType = qbcitSetValue;	
		m_queue[i].pinState = pinState;
		m_queue[i].startAfter = millis() + startAfter;

		m_queue[i].qState = qsDoIt;	
		m_queueRequests++;

		break;
	}
}

void ShpControlBinary::addQueueItemPressButton(int duration, unsigned long startAfter)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].qType = qbcitScenario;
		m_queue[i].startAfter = millis() + startAfter;

		m_queue[i].scenario[0].pinState = m_PinStateOn;
		m_queue[i].scenario[0].duration = duration;
		m_queue[i].scenario[1].pinState = m_PinStateOff;
		m_queue[i].scenario[1].duration = 0;

		m_queue[i].scenarioLen = 2;
		m_queue[i].scenarioPos = 0;

		m_queue[i].scenarioRepeatCount = 0;
		m_queue[i].scenarioRepeatPos = 0;
		m_queue[i].scenarioDurationExpire =  0;

		m_queue[i].qState = qsDoIt;	
		m_queueRequests++;

		break;
	}
}

void ShpControlBinary::addQueueItemFlash(int durations[], int durationsCnt, int repeatCnt, int totalDuration, unsigned long startAfter)
{
	for (int i = 0; i < SWITCH_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].qType = qbcitScenario;
		//m_queue[i].pinState = pinState;
		m_queue[i].startAfter = millis() + startAfter;

		for (int d = 0; d < durationsCnt; d++)
		{
			m_queue[i].scenario[d].pinState = (d % 2 == 0) ? m_PinStateOn : m_PinStateOff;
			m_queue[i].scenario[d].duration = durations[d];
		}
		m_queue[i].scenarioLen = durationsCnt;
		m_queue[i].scenarioPos = 0;

		m_queue[i].scenarioRepeatCount = repeatCnt;
		m_queue[i].scenarioRepeatPos = 0;
		
		m_queue[i].scenarioDurationExpire = 0;
		if (totalDuration)
		{
			m_queue[i].scenarioDurationExpire = m_queue[i].startAfter + totalDuration;
		}

		m_queue[i].qState = qsDoIt;	
		m_queueRequests++;

		break;
	}
}

void ShpControlBinary::runQueueItem(int i)
{
	if (m_queue[i].qType == qbcitSetValue)
	{
		m_queueRequests--;
		//Serial.printf ("Set value %d on pin %d\n", m_queue[i].pinState, m_pin);
		
		setPinState(m_queue[i].pinState);
		
		m_PinStateCurrent = m_queue[i].pinState;
		
		m_queue[i].qState = qsFree;
	}
	else
	if (m_queue[i].qType == qbcitScenario)
	{
		unsigned long now = millis();

		setPinState(m_queue[i].scenario[m_queue[i].scenarioPos].pinState);
		m_PinStateCurrent = m_queue[i].scenario[m_queue[i].scenarioPos].pinState;
		
		m_queue[i].scenarioPos++;
		if (m_queue[i].scenarioPos == m_queue[i].scenarioLen)
		{
			if (m_queue[i].scenarioRepeatCount)
			{
				m_queue[i].scenarioRepeatPos++;

				if (m_queue[i].scenarioRepeatPos < m_queue[i].scenarioRepeatCount)
				{
					m_queue[i].startAfter = now + m_queue[i].scenario[m_queue[i].scenarioPos - 1].duration;
					m_queue[i].scenarioPos = 0;
					m_queue[i].qState = qsDoIt;

					return;
				}
			}
			else if (m_queue[i].scenarioDurationExpire)
			{
				if (m_queue[i].scenarioDurationExpire > now)
				{
					m_queue[i].startAfter = now + m_queue[i].scenario[m_queue[i].scenarioPos - 1].duration;
					m_queue[i].scenarioPos = 0;
					m_queue[i].qState = qsDoIt;

					return;
				}
			}
		
			setPinState(m_PinStateOff);

			m_queueRequests--;
			m_queue[i].qState = qsFree;
		}
		else
		{
			m_queue[i].startAfter = now + m_queue[i].scenario[m_queue[i].scenarioPos - 1].duration;
			m_queue[i].qState = qsDoIt;
		}
	}
}

void ShpControlBinary::setPinState(uint8_t value)
{
	#ifdef SHP_GPIO_EXPANDER_I2C_H
	if (m_pinExpPortId)
	{
		if (m_gpioExpander)
		{
			m_gpioExpander->setPinState(m_pin, value);
			app->setValue(m_portId, (value == m_PinStateOn) ? "1" : "0", m_sendMode);
			log(shpllDebug, "set gpioExp pin state %d", value);
		}
		else
		{
			log(shpllError, "Invalid GPIO expander ");
		}
	}
	else
	#endif
	{
		digitalWrite(m_pin, value);
		app->setValue(m_portId, (value == m_PinStateOn) ? "1" : "0", m_sendMode);
		log(shpllDebug, "set pin state %d", value);
	}
}

void ShpControlBinary::loop()
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

