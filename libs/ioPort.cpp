extern SHP_APP_CLASS *app;

ShpIOPort::ShpIOPort () : m_portId(NULL), m_appPortIndex (0), m_valid(false), m_paused(false), m_pausedTo(0), m_sendMode(SM_LOOP), m_sendAsAction(0)
{
}

void ShpIOPort::init(JsonVariant portCfg)
{
	m_portId = portCfg["portId"].as<const char*>();

	if (portCfg.containsKey("sendAsAction"))
		m_sendAsAction = portCfg["sendAsAction"];

	//if (portCfg["valueTopic"])
	if (portCfg.containsKey("valueTopic"))
	{
		m_valueTopic = portCfg["valueTopic"].as<const char*>();
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

void ShpIOPort::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	/*
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
	*/
}

void ShpIOPort::shutdown()
{
}

int32_t ShpIOPort::stateLoad(int32_t valueNotExist /* = -1 */)
{
	char id[80] = "PS_";
	strcat(id, m_portId);

	app->m_prefs.begin(id);
	int32_t result = app->m_prefs.getInt("state", valueNotExist);
	app->m_prefs.end();

	return result;
}

void ShpIOPort::stateSave(int32_t state)
{
	char id[80] = "PS_";
	strcat(id, m_portId);

	app->m_prefs.begin(id);
	app->m_prefs.putInt("state", state);
	app->m_prefs.end();
}
