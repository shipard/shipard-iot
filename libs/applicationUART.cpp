
ApplicationUART::ApplicationUART()
{
}

void ApplicationUART::setup()
{

	Application::setup();


	Serial.println("UART setup");

}

boolean ApplicationUART::publish(const char *payload, const char *topic /* = NULL */)
{
	String data = topic;
	data.concat ("\n");
	data.concat (payload);
	
	Serial.println ("ApplicationUART::publish");
	Serial.println (data);
	
	//m_espClient->sendData(data, SHP_ENPT_MESSAGE, m_espClient->m_serverAddress.c_str());
  return true;
}

void ApplicationUART::publishData(uint8_t sendMode)
{
	if (sendMode == SM_NONE)
		return;
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}

	String data = m_deviceTopic.c_str();
	data.concat("\n");
	serializeJson(m_iotBoxInfo, data);

	Serial.println ("ApplicationUART::publishData");
	Serial.println (data);

	//m_espClient->sendData(data, SHP_ENPT_MESSAGE, m_espClient->m_serverAddress.c_str());
}

bool ApplicationUART::setIotBoxFromStoredCfg()
{
  /*
	app->m_prefs.begin("IotBox");
	String data = app->m_prefs.getString("config", "");
	app->m_prefs.end();

	if (data.length() != 0)
	{
		Serial.println("### ApplicationUART::setIotBoxFromStoredCfg ### ");
		Serial.println(data);
		setIotBoxCfg(data);
		return true;
	}
  */
	return false;
}

void ApplicationUART::loop()
{
	Application::loop();

	//if (m_espClient)
	//	m_espClient->loop();

	//Serial.println("test_uart");
	//xdelay(100);
}


#define PWR_PIN 4
#define LED_PIN 12
#define PIN_DTR     25

void ApplicationUART::checks()
{
	Application::checks();
	
  //Serial.println("checks_uart");
  //Serial.println(app->m_boxConfigLoaded);

  if (!app->m_boxConfigLoaded)
  {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);


    Serial.println("GSM ON start");
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(500);
    //digitalWrite(PWR_PIN, LOW);
    delay(4000);
    Serial.println("GSM ON done");

    Serial.println("init iot-box config...");
    const char *CFG = R""""(
      {
        "deviceNdx": 17,
        "deviceId": "test-gsm-uart",
        "deviceType": "oli-esp32-poe",
        "ioPorts": [
            {
                "type": "signal/gsm",
                "portId": "gsm",
                "valueTopic": "shp/readers/test-gsm/gsm",
                "speed": 2,
                "mode": 0,
                "pinRX": 26,
                "pinTX": 27
            }
        ]
      })"""";

      app->setIotBoxCfg(CFG);
  }
}
/*
		app->setIotBoxCfg(data);
		if (app->m_boxConfigLoaded)
		{
			writeIotBoxConfig(data);

			m_mode = SHP_ENS_IDLE;
		}	





void ShpEspNowClient::writeIotBoxConfig(String data)
{
	app->m_prefs.begin("IotBox");
	app->m_prefs.putString("config", data);
	app->m_prefs.end();
}



   "iotBoxCfg": {
        "deviceNdx": 17,
        "deviceId": "test-poe1",
        "deviceType": "oli-esp32-poe",
        "ioPorts": [
            {
                "type": "signal/gsm",
                "portId": "gsm",
                "valueTopic": "shp/readers/test-poe1/gsm",
                "speed": 5,
                "mode": 0,
                "pinRX": 33,
                "pinTX": 32
            }
        ]
    }

*/