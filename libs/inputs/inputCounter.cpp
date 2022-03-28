extern SHP_APP_CLASS *app;


ShpInputCounter::ShpInputCounter() : 
																	m_pin(-1),
																	m_publishCntr(false),
																	m_publishPPS(false),
																	m_publishPPM(false),
																	m_useCoef(shpCntrUseCoef_NO),
																	m_coef(0),
																	m_timeoutOnToOff(1000),
																	m_publishDelta(2),
																	m_checkIntervalOn(500),
																	m_valueCurrent(0),
																	m_valueSaved(0),
																	m_valueSent(0),
																	m_intervalSave(60*1000*60), // one hour
																	m_intervalSend(60*1000),    // one minute
																	m_intervalCheck(1000),
																	m_nextSaveCheck(0),
																	m_nextSendCheck(0),
																	m_nextCheck(0),
																	m_lastOnValue(0),
																	m_lastOnState(127),
																	m_nextCheckOn(0),
																	m_lastOffMillis(0),
																	m_lastPPSSec(127),
																	m_secsInMin(0),
																	m_lastPPSValue(0),
																	m_lastPublishPPS(0),
																	m_lastPublishPPM(0),
																	m_sendTimeout(180000),
																	m_lastSendMillisPPS(0),
																	m_lastSendMillisPPM(0)
{
	for (int i = 0; i < 60; i++)
		m_pps[i] = 0;
}

void ShpInputCounter::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "inputCounter",
		"portId": "cntr",
		"valueTopic": "shp/sensors/dev-esp32poe/cntr",
		"pin": 34,
		"timeoutOnToOff": "3",
		"publishCntr": 1,
		"publishPPS": 1,
		"publishPPM": 1,
		"useCoef": "1",
		"coef": "2"
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg.containsKey("pin"))
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- publishCntr
	if (portCfg.containsKey("publishCntr"))
		m_publishCntr = portCfg["publishCntr"];

	// -- publishPPS
	if (portCfg.containsKey("publishPPS"))
		m_publishPPS = portCfg["publishPPS"];

	// -- publishPPM
	if (portCfg.containsKey("publishPPM"))
		m_publishPPM = portCfg["publishPPM"];

	// -- publishDelta	
	if (portCfg["publishDelta"] != nullptr)
		m_publishDelta = portCfg["publishDelta"];
	if (m_publishDelta < 0 || m_publishDelta > 100)	
		m_publishDelta = 2;

	// -- useCoef
	if (portCfg.containsKey("useCoef"))
		m_useCoef = portCfg["useCoef"];

	// -- coef
	if (portCfg.containsKey("coef"))
		m_coef = portCfg["coef"];

	// -- timeout ON --> OFF
	if (portCfg.containsKey("timeoutOnToOff"))
		m_timeoutOnToOff = (int)portCfg["timeoutOnToOff"] * 1000;

	if (!m_timeoutOnToOff)
		m_timeoutOnToOff = 1000;


	m_valueTopicOn = m_valueTopic + "_ON";	
	m_valueTopicPPS = m_valueTopic + "_PPS";	
	m_valueTopicPPM = m_valueTopic + "_PPM";

	m_prefsId = "cv_";
	m_prefsId.concat(m_portId);

	m_valueCurrent = loadValue();
	m_valueSaved = m_valueCurrent;
	m_lastOnValue = m_valueCurrent;
	m_lastPPSValue = m_valueCurrent;
	for (int i = 0; i < 60; i++)
		m_pps[i] = m_valueCurrent;

	pinMode(m_pin, INPUT);
	attachInterrupt(m_pin, std::bind(&ShpInputCounter::onPinChange, this, m_pin), RISING);
}

void ShpInputCounter::onPinChange(int pin)
{
	m_valueCurrent++;
}

void ShpInputCounter::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	if (!subCmd || strcmp(subCmd, "set") != 0)
	{
		return;
	}

	if (length < shpInputCouter_a)
	{
		char number[shpInputCouter_a + 1] = "";
		strncpy (number, (const char*)payload, length);

		char *err;
		shpInputCouter_t v = strtoll(number, &err, 10);
		if (*err)
		{
			return;
		}

		m_valueCurrent = v;
		m_valueSaved = v - 1;
		m_valueSent = v - 1;
	}
}

shpInputCouter_t ShpInputCounter::loadValue()
{
	if (!m_publishCntr)
		return 0;

	char number[shpInputCouter_a] = "";

	app->m_prefs.begin("IotBox");
	size_t nl = app->m_prefs.getString(m_prefsId.c_str(), number, shpInputCouter_a);
	app->m_prefs.end();

	if (!nl)
		return 0;

	char *err;
	shpInputCouter_t v = strtoll(number, &err, 10);
	if (*err)
	{
		Serial.println("LOAD FAILED");

		return 0;
	}

	//Serial.printf("LOAD: `%lld`\n", v);

	return v;
}

void ShpInputCounter::saveValue(shpInputCouter_t value)
{
	if (!m_publishCntr)
		return;

	char number[shpInputCouter_a];
	sprintf(number, shpInputCouter_f, value);

	app->m_prefs.begin("IotBox");
	app->m_prefs.putString(m_prefsId.c_str(), number);
	app->m_prefs.end();

	m_valueSaved = value;	

	//Serial.printf("SAVE VALUE: `%s`\n", number);
}

void ShpInputCounter::save()
{
	if (!m_publishCntr)
		return;

	shpInputCouter_t v = m_valueCurrent;
	if (v == m_valueSaved)
		return;

	saveValue(v);	
}

void ShpInputCounter::send()
{
	if (!m_publishCntr)
		return;

	shpInputCouter_t v = m_valueCurrent;
	if (v == m_valueSent)
		return;

	char number[shpInputCouter_a];
	sprintf(number, shpInputCouter_f, v);
	app->publish(number, m_valueTopic.c_str());

	m_valueSent = v;	
}

void ShpInputCounter::shutdown()
{
	save();

	ShpIOPort::shutdown();
}

void ShpInputCounter::checkPPS()
{
	int secs = millis() / 1000;
	uint8_t sec = secs % 60;

	m_pps[sec] = m_valueCurrent;

	m_lastPPSValue = m_pps[sec];

	if (m_lastPPSSec == 127)
	{ // init - first use
		m_lastPPSSec = sec;
		m_secsInMin = 1;
		return;
	}

	uint8_t nextSec = (sec == 59) ? 0 : sec + 1;
	uint8_t nns = nextSec;
	while (1)
	{
		nns++;
		if (nns == 60)
			nns = 0;
		if (m_pps[nns] < m_pps[nextSec])
		{
			nextSec = nns;
			continue;
		}
		break;
	}
	uint8_t nextSecUsed = nns;

	int cntSecsPps = (sec > m_lastPPSSec) ? sec - m_lastPPSSec : m_lastPPSSec - sec - 58;
	if (cntSecsPps <= 0)
		cntSecsPps = 1;

	if (!m_lastOnState)
	{
		if (m_secsInMin > cntSecsPps)
			m_secsInMin -= cntSecsPps;
	}
	else	
		m_secsInMin += cntSecsPps;

	if (m_secsInMin > 60)
		m_secsInMin = 60;

	int pps = (m_pps[sec] - m_pps[m_lastPPSSec]) / cntSecsPps;

	int ppm = 0;
	if (m_secsInMin > 3 && m_lastOnState)
		ppm = ((m_pps[sec] - m_pps[nextSecUsed]) / m_secsInMin) * 60;

	if (m_coef && m_useCoef)
	{
		if (m_useCoef == shpCntrUseCoef_DIVIDE)
		{
			pps /= m_coef;
			ppm /= m_coef;	
		}
		else if (m_useCoef == shpCntrUseCoef_MULTIPLY)
		{
			pps *= m_coef;
			ppm *= m_coef;	
		}
	}

	//Serial.printf("sec: %d, pps=%d, ppm=%d, cntSecsPps=%d, m_secsInMin=%d\n", int(sec), pps, ppm, cntSecsPps, (int)m_secsInMin);
	unsigned long now = millis();
	if (m_publishPPS)
	{
		int delta = abs(m_lastPublishPPS - pps);
		int deltaPerc = (pps) ? ((delta * 100) / pps) : delta;
		//Serial.printf("DELTA-PPS: last=%d,  delta=%d, perc=%d\n", m_lastPublishPPS, delta, deltaPerc);
		if (deltaPerc > m_publishDelta || (now - m_lastSendMillisPPS) > m_sendTimeout)
		{
			char buffer[20];
			itoa(pps, buffer, 10);
			app->publish(buffer, m_valueTopicPPS.c_str());
			m_lastPublishPPS = pps;
			m_lastSendMillisPPS = now;
		}
	}

	if (m_publishPPM)
	{
		int delta = abs(m_lastPublishPPM - ppm);
		int deltaPerc = (ppm) ? ((delta * 100) / ppm) : delta;
		//Serial.printf("DELTA-PPM: last=%d,  delta=%d, perc=%d\n", m_lastPublishPPM, delta, deltaPerc);
		if (deltaPerc > m_publishDelta || (now - m_lastSendMillisPPM) > m_sendTimeout)
		{
			char buffer[20];
			itoa(ppm, buffer, 10);
			app->publish(buffer, m_valueTopicPPM.c_str());
			m_lastPublishPPM = ppm;
			m_lastSendMillisPPM = now;
		}
	}

	m_lastPPSSec = sec;
}

void ShpInputCounter::loop()
{
	unsigned long now = millis();

	if (now > m_nextCheckOn)
	{
		volatile shpInputCouter_t currentValue = m_valueCurrent;
		uint8_t onState = ((currentValue == m_lastOnValue) ? 0 : 1);
		//Serial.printf("currentValue=%lld, lastOnValue=%lld, onState=%d, lastOnState=%d\n", currentValue, m_lastOnValue, (int)onState, (int)m_lastOnState);
			
		if (onState)
		{
			if (m_lastOnState != onState)
				app->publish("1", m_valueTopicOn.c_str());

			m_lastOnState = onState;
			m_lastOffMillis = now;
		}
		else
		{
			if (now - m_lastOffMillis > m_timeoutOnToOff)
			{
				m_lastOffMillis = now;

				if (m_lastOnState != onState)
				{
					app->publish(onState ? "1" : "0", m_valueTopicOn.c_str());
				}
				
				m_lastOnState = onState;
			}
		}

		m_lastOnValue = currentValue;
		m_nextCheckOn = now + m_checkIntervalOn;
		return;
	}


	if (now < m_nextCheck)
		return;

	checkPPS();

	if (now > m_nextSendCheck)
	{
		send();	
		m_nextSendCheck = now + m_intervalSend;
	}

	if (now > m_nextSaveCheck)
	{
		save();	
		m_nextSaveCheck = now + m_intervalSave;
	}

	m_nextCheck = now + m_intervalCheck;
}

