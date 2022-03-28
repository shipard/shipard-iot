extern SHP_APP_CLASS *app;

ShpIOPort::ShpIOPort () : m_portId(NULL), m_appPortIndex (0), m_valid(false), m_paused(false), m_pausedTo(0), m_sendMode(SM_LOOP), m_sendAsAction(0)
{
}

void ShpIOPort::init(JsonVariant portCfg)
{
	m_portId = portCfg["portId"];

	if (portCfg.containsKey("sendAsAction"))
		m_sendAsAction = portCfg["sendAsAction"];

	if (portCfg["valueTopic"])
	{
		m_valueTopic = (const char*)portCfg["valueTopic"];
	}
	else
	{
		m_valueTopic = MQTT_TOPIC_THIS_DEVICE "/sensors/";
		m_valueTopic.concat(app->m_deviceId);
		m_valueTopic.concat("_");
		m_valueTopic.concat(m_portId);
	}

	//Serial.printf("VALUE TOPIC: %s \n", m_valueTopic.c_str());
}

void ShpIOPort::init2()
{
}

void ShpIOPort::log(uint8_t level, const char* format, ...)
{
	char msg[512];
	sprintf(msg, "[%s][%s] ", SHP_LOG_LEVEL_NAMES[level - 1], m_portId);

	va_list argptr;
  va_start(argptr, format);
  vsnprintf(msg + strlen(msg), 512, format, argptr);
  va_end(argptr);

	app->log(msg, level);
}

void ShpIOPort::loop()
{
	if (m_paused && millis() > m_pausedTo)
		m_paused = false;
}

void ShpIOPort::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	if (subCmd && strcmp(subCmd, "pause") == 0)
	{
		if (strncmp((const char*)payload, "on", 2) == 0)
			m_paused = true;
		else if (strncmp((const char*)payload, "off", 3) == 0)
			m_paused = false;
		else
		{ // pause <msecs>
			if (length > 7)
			{
				// TODO: error message
				return;			
			}

			char b[10];
			strncpy(b, (const char*)payload, length);
			b[length] = 0;

			int n = atoi(b);
			m_paused = true;
			m_pausedTo = millis() + n;
		}
	}
}

void ShpIOPort::shutdown()
{
}
