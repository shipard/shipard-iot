ApplicationCAN::ApplicationCAN() : m_client(nullptr)
{
}

void ApplicationCAN::setup()
{
	Application::setup();

	Serial.println(WiFi.macAddress());

	Serial.println("setup");

	macHostName = WiFi.macAddress();
	macHostName.replace(':', '-');
	macHostName.toLowerCase();

	m_client = new ShpClientCAN();
  m_client->init();
}

void ApplicationCAN::doFwUpgradeRequest(String payload)
{
	//Serial.println("FW UPGRADE REQUEST:");
	//Serial.println(payload);
	if (m_client)
		m_client->doFwUpgradeRequest(payload);
}

boolean ApplicationCAN::publish(const char *payload, const char *topic /* = NULL */)
{
	Serial.println("ApplicationCAN::publish:");
	Serial.println(topic);
	Serial.println(payload);
	Serial.println("---");

	m_client->publish(payload, topic);

  return true;
}

void ApplicationCAN::publishData(uint8_t sendMode)
{
	Serial.println("publishData");
	if (sendMode == SM_NONE)
		return;
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}

	String payload;
	serializeJson(m_iotBoxInfo, payload);

	app->publish(payload.c_str(), m_deviceTopic.c_str());
}

void ApplicationCAN::loop()
{
	Application::loop();

	if (m_client)
	  m_client->loop();
}

void ApplicationCAN::checks()
{
	Application::checks();

	//Serial.println ("ApplicationDSEN::checks");
	/*
	if (!m_espClient->m_paired && !m_espClient->m_pairingMode)
	{
		Serial.println("do check espnow client...");
		m_espClient->readServerAddress();
		if (!m_espClient->m_paired)
			m_espClient->doPair();

		delay(1000);
	}
	*/
}
