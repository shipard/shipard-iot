
ApplicationUARTWiFi::ApplicationUARTWiFi()
{
  m_wifiInitialized = false;
}

void ApplicationUARTWiFi::setup()
{
	ApplicationLan::setup();
}


void ApplicationUARTWiFi::loop()
{
	Application::loop();

	//if (m_espClient)
	//	m_espClient->loop();

	//Serial.println("test_uart");
	//xdelay(100);
}


void ApplicationUARTWiFi::checks()
{
	ApplicationLan::checks();

  if (!m_wifiInitialized)
  {
    if (m_wifiSSID.length() != 0 && m_wifiPassword.length() != 0)
    {
      Serial.println("INIT WIFI");
		  WiFi.onEvent(WiFiEvent2);

      WiFi.begin(m_wifiSSID.c_str(), m_wifiPassword.c_str());
      WiFi.setSleep(false);


      int cnt = 0;
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(1000);
        Serial.print(".");
        cnt++;

        if (cnt == 10)
          break;
      }
      Serial.println("");

      if (WiFi.status() != WL_CONNECTED)
        Serial.println("CAM WiFi NOT connected");
      else
      {
        Serial.println("CAM WiFi connected");
        m_wifiInitialized = true;
      }
    }
  }
}
