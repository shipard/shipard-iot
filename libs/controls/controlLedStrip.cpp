extern Application *app;


ShpControlLedStrip::ShpControlLedStrip() : 
																				m_pin(-1),
																				m_cntLeds(0),
																				m_strip(NULL),
																				m_ws2812fx(NULL)
{
}

void ShpControlLedStrip::init(JsonVariant portCfg)
{
	/* config format:
	 * ----------------------------
	{
		"type": "controlLedStrip",
		"portId": "uio-6-5",
		"pin": 16,
		"cntLeds": 8
  }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg["pin"] != nullptr)
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	// -- cntLeds
	if (portCfg["cntLeds"] != nullptr)
		m_cntLeds = portCfg["cntLeds"];

	if (m_cntLeds <= 0)
		return;

	pinMode(m_pin, OUTPUT);

	m_ws2812fx = new WS2812FX(m_cntLeds, m_pin, NEO_GRB + NEO_KHZ800);
	  
	m_ws2812fx->init();
  m_ws2812fx->setBrightness(250);
  m_ws2812fx->setSpeed(4000);
	m_ws2812fx->setColor(0x101010);


  m_ws2812fx->setMode(/*FX_MODE_LARSON_SCANNER*/FX_MODE_SCAN);
  m_ws2812fx->start();
}

void ShpControlLedStrip::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	addQueueItemFromMessage(topic, payload, length);
}

void ShpControlLedStrip::addQueueItemFromMessage(const char* topic, byte* payload, unsigned int length)
{
	/* PAYLOADS:
	 * mode:
	 * <mode>:<speed>:<color1>[:<color2>][:<color3>];
	 * 
	 */

	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	char *oneItem = NULL;
	int paramNdx = 0;

	int mode = LEDSTRIP_INVALID_MODE_NAME;
	long speed = 0;
	uint32_t colors[3] = {0, 0, 0};
	int cntColors = 0;

	
	char *err;
	oneItem = strtok(payloadStr.begin(), ":");
	while (oneItem != NULL)
	{
		if (paramNdx == 0)	
		{ // mode
			mode = modeFromString(oneItem);

		}
		else if (paramNdx == 1)
		{ // speed
			if (oneItem[0] == 0)
				speed = 0;
			else
			{
				speed = strtol(oneItem, &err, 10);
				if (*err) 
				{
					return;
				}
			}
		}
		else if (paramNdx > 1 && paramNdx < 5)
		{ // color
			if (oneItem[0] == 0)
				colors[cntColors] = 0;
			else
			{
				colors[cntColors] = strtol(oneItem, &err, 16);
				if (*err) 
				{
					return;
				}
			}
			cntColors++;
		}

		oneItem = strtok (NULL, ":");
		paramNdx++;

		if (paramNdx > 4)
			break;
	}

	if (mode == LEDSTRIP_INVALID_MODE_NAME)
		return;


	if (cntColors == 1)
		m_ws2812fx->setColor(colors[0]);
	else if (cntColors > 1)	
		m_ws2812fx->setColors(0, colors);

	if (speed != 0)
		m_ws2812fx->setSpeed(speed);

  m_ws2812fx->setMode(mode);
}

uint8_t ShpControlLedStrip::modeFromString(const char *mode)
{
	static const ModeNames mn[LEDSTRIP_COUNT_MODES] = {
		{"static", FX_MODE_STATIC},
		{"blink", FX_MODE_BLINK},
		{"breath", FX_MODE_BREATH},
		{"scan", FX_MODE_SCAN},
		{"dual-scan", FX_MODE_DUAL_SCAN},
		{"larson-scanner", FX_MODE_LARSON_SCANNER}
	};


	for (int i = 0; i < LEDSTRIP_COUNT_MODES; i++)
	{
		if (strcmp(mn[i].name, mode) == 0)
			return mn[i].mode;
	}

	return LEDSTRIP_INVALID_MODE_NAME;
}

void ShpControlLedStrip::setColorAll (uint32_t color)
{
	for (int i = 0; i < m_cntLeds; i++)
		m_strip->setPixelColor(i, color);

	m_strip->show();	
}

void ShpControlLedStrip::loop()
{
	if (!m_ws2812fx)
		return;

	m_ws2812fx->service();
}

