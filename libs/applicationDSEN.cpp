#ifdef ESP32
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif




void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */


ApplicationDSEN::ApplicationDSEN() :
																		m_espClient(NULL),
																		m_wakeUpFromSleep(false)
{
}

void ApplicationDSEN::setup()
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	Application::setup();

	print_wakeup_reason();
	esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
	if (wakeup_reason)
		m_wakeUpFromSleep = true;

	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

	Serial.println(WiFi.macAddress());

	Serial.println("setup");

	macHostName = WiFi.macAddress();
	macHostName.replace(':', '-');
	macHostName.toLowerCase();

	m_espClient = new ShpEspNowClient();
	m_espClient->init();
}

boolean ApplicationDSEN::publish(const char *payload, const char *topic /* = NULL */)
{
	String data = topic;
	data.concat ("\n");
	data.concat (payload);
	
	//Serial.println ("ApplicationDSEN::publish");
	//Serial.println (data);
	
	m_espClient->sendData(data, SHP_ENPT_MESSAGE, m_espClient->m_serverAddress.c_str());
  return true;
}

void ApplicationDSEN::publishData(uint8_t sendMode)
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

	//Serial.println ("ApplicationDSEN::publishData");
	//Serial.println (data);

	m_espClient->sendData(data, SHP_ENPT_MESSAGE, m_espClient->m_serverAddress.c_str());
}

bool ApplicationDSEN::setIotBoxFromStoredCfg()
{
	app->m_prefs.begin("IotBox");
	String data = app->m_prefs.getString("config", "");
	app->m_prefs.end();

	if (data.length() != 0)
	{
		//Serial.println("### ApplicationDSEN::setIotBoxFromStoredCfg ### ");
		//Serial.println(data);
		setIotBoxCfg(data);
		return true;
	}

	return false;
}

void ApplicationDSEN::loop()
{
	Application::loop();

	if (m_espClient)
		m_espClient->loop();

	unsigned long now = millis();
	if ((m_wakeUpFromSleep && now > 500) || (!m_wakeUpFromSleep && now > 10 * 1000))
	{
		Serial.printf("GOING TO SLEEP AFTER %ld millis\n", now);
		esp_wifi_stop();
		//esp_bt_controller_disable();
		delay(100);
		esp_deep_sleep_start();
	}
	//Serial.println("test");
	//delay(100);
}

void ApplicationDSEN::checks()
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
