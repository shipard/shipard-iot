extern ApplicationLan *app;




ShpWiFiConnector::ShpWiFiConnector()
{
  start();
}


void ShpWiFiConnector::start()
{
  // -- load wifi config
  app->m_prefs.begin("ibCfgWiFi");
  String wifiCfgData = app->m_prefs.getString("config");
  app->m_prefs.end();
  DeserializationError error = deserializeJson(m_wifiConfig, wifiCfgData.c_str());
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    m_wifiConfig.clear();

    return;
  }

  setWiFiSSID(0);


  /*
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    cnt++;

    if (cnt >= 40)
    {
      ESP.restart();
      break;
    }
  }
  Serial.println("");

  Serial.println("wifi connected!");
  */
}

void ShpWiFiConnector::setWiFiSSID(uint8_t index)
{
  delay(3000);
	uint8_t count = m_wifiConfig.size();
  for (uint8_t i = 0; i < count; i++)
  {
    JsonVariant oneSSID = m_wifiConfig[i];
    const char *ssid = (const char*)oneSSID["ssid"];
    const char *password = (const char*)oneSSID["password"];
    //Serial.printf("###### Connecting to WiFi SSID [%d]: %s, password: %s\n", count, ssid, password);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    return;
  }
}

void ShpWiFiConnector::stop()
{
}

void ShpWiFiConnector::loop()
{
}

