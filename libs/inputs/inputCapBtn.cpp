extern SHP_APP_CLASS *app;


ShpInputCapBtn::ShpInputCapBtn() :
																	m_pin(-1),
                                  m_treshold(40),
                                  m_ledStripPixel(0),
																	m_measureInterval(20),
																	m_nextMeasure(0),
                                  m_debounceDelay(100),
                                  m_lastDebounceTime(0),
                                  m_touchState(false),
                                  m_lastTouchState(false)
{
}

void ShpInputCapBtn::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "inputCapBtn",
		"portId": "uio-6-5",
		"sendValue": 0,
		"measureInterval": 1000,
		"pin": 16,
    "treshold": 50
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg.containsKey("pin"))
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- treshold
	if (portCfg.containsKey("treshold"))
		m_treshold = portCfg["treshold"];

	if (m_treshold <= 0 || m_treshold > 900000)
		m_treshold = 40;

	// -- portId buzzer
	if (portCfg.containsKey("portIdBuzzer"))
	{
		m_portIdBuzzer.concat((const char*)portCfg["portIdBuzzer"]);
	}

  // -- LED strip
	if (portCfg.containsKey("portIdLedStrip"))
		m_portIdLedStrip.concat((const char*)portCfg["portIdLedStrip"]);
	if (portCfg.containsKey("ledStripPixel"))
		m_ledStripPixel = portCfg["ledStripPixel"];
	if (portCfg.containsKey("ledStripPixelColor"))
    m_ledStripPixelColor.concat((const char*)portCfg["ledStripPixelColor"]);

  Serial.println("treshold: ");
  Serial.println(m_treshold);

	m_sendMode = SM_LOOP;
}

void ShpInputCapBtn::loop()
{
	ShpIOPort::loop();

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	int newValue = touchRead(m_pin);
  //Serial.println(newValue);

  bool reading = newValue < m_treshold;

  if (reading != m_lastTouchState)
  {
    m_lastDebounceTime = millis();
  }

  if ((millis() - m_lastDebounceTime) > m_debounceDelay)
	{
    //Serial.println(newValue);
    if (reading != m_touchState)
    {
      m_touchState = reading;

      char buffer[] = "1";
      if (!m_touchState)
        buffer[0] = '0';

      if (m_sendAsAction)
        app->publishAction(m_portId, buffer);
      else
      {
        if (!app->publish(buffer, m_valueTopic.c_str()))
          return;
      }

      if (m_touchState && m_portIdBuzzer != "")
      {
        ShpIOPort *ioPortBuzzer = app->ioPort(m_portIdBuzzer.c_str());
        if (ioPortBuzzer)
        {
          ioPortBuzzer->onMessage((byte*)"P50", 3, NULL);
        }
      }
      if (m_portIdLedStrip != "")
      {
        ShpIOPort *ioPortLedStrip = app->ioPort(m_portIdLedStrip.c_str());
        if (ioPortLedStrip)
        {
          if (m_touchState)
          {
            char ledCmd[80];
            sprintf(ledCmd, "pixel:%d:%s", m_ledStripPixel, m_ledStripPixelColor.c_str());
            ioPortLedStrip->onMessage((byte*)ledCmd, strlen(ledCmd), NULL);
          }
          else
          {
            ioPortLedStrip->onMessage((byte*)"resume:0", 9, NULL);
          }
        }
      }
    }
	}
  m_lastTouchState = reading;

	m_nextMeasure = millis() + m_measureInterval;
}
