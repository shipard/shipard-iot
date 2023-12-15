extern SHP_APP_CLASS *app;


ShpControlLedStrip::ShpControlLedStrip() :
																				m_pin(-1),
																				m_cntLeds(0),
																				m_colorMode(0),
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

	// -- color mode
	if (portCfg.containsKey("colorMode"))
		m_colorMode = portCfg["colorMode"];
	if (m_colorMode >= LED_STRIP_COLOR_MODES_CNT)
		m_colorMode = 0;

	pinMode(m_pin, OUTPUT);

	m_ws2812fx = new WS2812FX(m_cntLeds, m_pin, LED_STRIP_COLOR_MODES[m_colorMode] + NEO_KHZ800);

	m_ws2812fx->init();
  m_ws2812fx->setBrightness(100);
  m_ws2812fx->setSpeed(4000);
	m_ws2812fx->setColor(0x101010);

  m_ws2812fx->setMode(/*FX_MODE_LARSON_SCANNER*/FX_MODE_SCAN);
  m_ws2812fx->start();
}

void ShpControlLedStrip::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	addQueueItemFromMessage(payload, length);
}

void ShpControlLedStrip::addQueueItemFromMessage(byte* payload, unsigned int length)
{
	/* PAYLOADS:
	 * mode:
	 * <mode>:<speed>:<color1>[:<color2>][:<color3>];
	 *
	 */

	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	String m_lastPayload;
	for (int i = 0; i < length; i++)
		m_lastPayload.concat((char)payload[i]);

	//String pp = payloadStr;

	//if (m_lastPayload == payloadStr)
	//	return;

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
	{
		//Serial1.println("invalid mode");
		return;
	}

	if (mode == LEDSTRIP_SET_BRIGHTNESS)
	{
		if (speed < 0)
			speed = 0;
		else
			if (speed > 255)
				speed = 255;
		m_ws2812fx->setBrightness(speed);
	}
	else if (mode == LEDSTRIP_SET_SPEED)
	{
		if (speed < 0)
			speed = 0;
		else
			if (speed > 65534)
				speed = 65534;
		m_ws2812fx->setSpeed(speed);
	}
	else if (mode == LEDSTRIP_SET_PIXEL)
	{
		Serial.println("PIXEL");
		Serial.println(speed);
		Serial.println(colors[0]);

		if (speed < 0)
			speed = 0;
		else
			if (speed > m_cntLeds)
				speed = m_cntLeds;
		m_ws2812fx->setPixelColor(speed, colors[0]);
		if (m_ws2812fx->isRunning())
			m_ws2812fx->pause();
		m_ws2812fx->show();
	}
	else if (mode == LEDSTRIP_SET_PAUSE)
	{
		if (m_ws2812fx->isRunning())
			m_ws2812fx->pause();
	}
	else if (mode == LEDSTRIP_SET_RESUME)
	{
		if (!m_ws2812fx->isRunning())
		{
			m_ws2812fx->resume();
			m_ws2812fx->show();
		}
	}
	else
	{
		if (cntColors == 1)
			m_ws2812fx->setColor(colors[0]);
		else if (cntColors > 1)
			m_ws2812fx->setColors(0, colors);

		if (speed != 0)
			m_ws2812fx->setSpeed(speed);

		m_ws2812fx->setMode(mode);

		if (!m_ws2812fx->isRunning())
			m_ws2812fx->resume();
	}
}

uint8_t ShpControlLedStrip::modeFromString(const char *mode)
{
	static const ModeNames mn[LEDSTRIP_COUNT_MODES] = {
		{"static", FX_MODE_STATIC},
		{"blink", FX_MODE_BLINK},
		{"breath", FX_MODE_BREATH},
		{"comet", FX_MODE_COMET},
		{"scan", FX_MODE_SCAN},
		{"dual-scan", FX_MODE_DUAL_SCAN},
		{"larson-scanner", FX_MODE_LARSON_SCANNER},

		{"fire-flicker", FX_MODE_FIRE_FLICKER},
		{"fire-flicker-soft", FX_MODE_FIRE_FLICKER_SOFT},
		{"fire-flicker-intense", FX_MODE_FIRE_FLICKER_INTENSE},

		{"fireworks", FX_MODE_FIREWORKS},
		{"fireworks-random", FX_MODE_FIREWORKS_RANDOM},

		{"rain", FX_MODE_RAIN},

		{"rainbow", FX_MODE_RAINBOW},
		{"rainbow-cycle", FX_MODE_RAINBOW_CYCLE},

		{"running-color", FX_MODE_RUNNING_COLOR},
		{"running-lights", FX_MODE_RUNNING_LIGHTS},
		{"running-random", FX_MODE_RUNNING_RANDOM},

		{"twinklefox", FX_MODE_TWINKLEFOX},
		{"circus-combustus", FX_MODE_CIRCUS_COMBUSTUS},
		{"halloween", FX_MODE_HALLOWEEN},

		{"chase-blackout", FX_MODE_CHASE_BLACKOUT},
		{"chase-rainbow-white", FX_MODE_CHASE_RAINBOW_WHITE},
		{"chase-flash", FX_MODE_CHASE_FLASH},
		{"chase-flash-random", FX_MODE_CHASE_FLASH_RANDOM},

		{"bicolor-chase", FX_MODE_BICOLOR_CHASE},
		{"tricolor-chase", FX_MODE_TRICOLOR_CHASE},


		{"brightness", LEDSTRIP_SET_BRIGHTNESS},
		{"speed", LEDSTRIP_SET_SPEED},
		{"pixel", LEDSTRIP_SET_PIXEL},
		{"pause", LEDSTRIP_SET_PAUSE},
		{"resume", LEDSTRIP_SET_RESUME}
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

