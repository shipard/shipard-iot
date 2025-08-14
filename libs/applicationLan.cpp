#ifdef SHP_ETH
	#include <ETH.h>
#endif

#ifdef SHP_ETH_LAN8720
	#ifdef ETH_CLK_MODE
		#undef ETH_CLK_MODE
	#endif
	#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
	#define ETH_POWER_PIN   -1
	#define ETH_TYPE        ETH_PHY_LAN8720
	#define ETH_ADDR        0
	#define ETH_MDC_PIN     23
	#define ETH_MDIO_PIN    18
#endif

#include <netdb.h>
#include <lwip/dns.h>
#include <HTTPClient.h>
#include <WiFiType.h>

#include <WiFiClientSecure.h>


bool ApplicationLan::eth_connected = false;


void WiFiEvent2(WiFiEvent_t event)
{
	Serial.println("WiFiEvent2");
	Serial.println(event);
  switch (event)
  {
		#ifdef SHP_ETH
    case ARDUINO_EVENT_ETH_START:
			Serial.print("ETH Started, MAC: ");
			Serial.println(ETH.macAddress());
      ETH.setHostname(app->m_deviceId.c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.print("ETH Connected, MAC: ");
			Serial.println(ETH.macAddress());
      break;
		#endif
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi Connected");
      break;
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi Disconnected");
			break;
		#ifdef SHP_ETH
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH GOT IP; MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
			ApplicationLan::eth_connected = true;
      app->IP_Got();
      break;
			#endif
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
		  Serial.print("WiFi MAC: ");
      Serial.print(WiFi.macAddress());
			Serial.print(", IPv4: ");
      Serial.println(WiFi.localIP());
			ApplicationLan::eth_connected = true;
      app->IP_Got();
			break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
		case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("ETH Disconnected");
      ApplicationLan::eth_connected = false;
			app->IP_Lost();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ApplicationLan::eth_connected = false;
			app->IP_Lost();
      break;
    default:
      break;
  }
}

#ifdef SHP_MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
	app->onMqttMessage(topic, payload, length);
}
#endif


ApplicationLan::ApplicationLan() :
																		#ifdef SHP_MQTT
																		mqttClient (NULL),
																		#endif
																		#ifdef SHP_WIFI
																		m_wifiConnector(NULL),
																		#endif
																		m_networkInfoInitialized(false),
																		m_mqttReconnectAttempAfter(0),
																		m_loadConfigAfter(0)
{
}

void ApplicationLan::init()
{
	m_prefs.begin("IotBox");
	bool doUpgrade = m_prefs.getBool("doUpgrade", false);
	int fwLength = m_prefs.getInt("fwLength", 0);
	String fwUrl = m_prefs.getString("fwUrl", "");
	m_prefs.end();

	if (doUpgrade)
	{
		//Serial.println("**** doFwUpgradeRun ****");
		log(shpllStatus, "[OTA UPGRADE] upgrade start: %d bytes from URL `%s`", fwLength, fwUrl.c_str());
		ShpOTAUpdate *ota = new ShpOTAUpdate();
		ota->doFwUpgradeRun(fwLength, fwUrl);
		delete ota;

		return;
	}

	// -- wait for mqtt client
	#ifdef SHP_MQTT
	int tryCount = 0;
	while (tryCount < 10)
	{
		checkMqtt();
		if (mqttClient->connected())
			break;
		Serial.println("WAIT FOR MQTT CLIENT");
		delay(200);
		tryCount++;
	}
	#endif

	Application::init();
}

void ApplicationLan::init2IOPorts()
{
	Application::init2IOPorts();

	#ifdef SHP_MQTT
  if (!mqttClient || !mqttClient->connected())
		return;
	Serial.printf("[MQTT] 2 subscribe routed topics: %d\n", m_routedTopicsCount);
	for (int i = 0; i < m_routedTopicsCount; i++)
	{
		Serial.print("SUBSCRIBE 2 ROUTED TOPIC: ");
		Serial.println(m_routedTopics[i].topic);
		String st = m_routedTopics[i].topic;
		st.concat ("#");
		mqttClient->subscribe(st.c_str());
	}
	#endif
}

void ApplicationLan::checks()
{
	Application::checks();

	if (!eth_connected)
	{
		//Serial.println("ETH NOT CONNECTED");
		return;
	}

	if (!m_networkInfoInitialized)
	{
		initNetworkInfo();
		return;
	}

	if (!m_boxConfigLoaded)
	{
		if (m_loadConfigAfter < millis())
			loadBoxConfig();

		if (millis() > 60 * 1000)
		{
			ESP.restart();
			return;
		}
		return;
	}

	#ifdef SHP_MQTT
	checkMqtt();
	#endif
}

#ifdef SHP_MQTT
void ApplicationLan::checkMqtt()
{
	if (m_mqttServerHostName.length() == 0)
	{
		app->setHBLedStatus(hbLEDStatus_Unconfigured);
		app->log(shpllError, "MQTT server host name not set, config loading failed");
		m_mqttReconnectAttempAfter = millis() + 5000;
		return;
	}

  if (!eth_connected || !mqttClient || !m_boxConfigLoaded || mqttClient->state() == MQTT_CONNECTED)
	{
		m_mqttReconnectAttempAfter = millis() + 5000;
		return;
	}

	mqttClient->setServer(m_mqttServerHostName.c_str(), 1883);
	Serial.println("[MQTT] connect to server!");
	Serial.println(m_mqttServerHostName.c_str());

	/*
	String id = (const char*)m_boxConfig["deviceId"];
	id.concat (millis());
	id.concat(rand());
	*/

Serial.println("[MQTT] connect 0!");
	setHBLedStatus(hbLEDStatus_WaitForCfg);
Serial.println("[MQTT] connect 1!");
	mqttClient->connect((const char*)m_boxConfig["deviceId"], m_logTopic.c_str(), 0, 0, "disconnect");
Serial.println("[MQTT] connect 2!");

	if (mqttClient->state() != MQTT_CONNECTED)
	{
		m_mqttReconnectAttempAfter = millis() + 5000;
		return;
	}
Serial.println("[MQTT] connect 3!");

	String dst = m_deviceTopic + "#";
	mqttClient->subscribe(dst.c_str());
	Serial.printf("subscribe device topic: `%s`\n", dst.c_str());

	Serial.printf("[MQTT] subscribe routed topics: %d\n", m_routedTopicsCount);
	for (int i = 0; i < m_routedTopicsCount; i++)
	{
		String st = m_routedTopics[i].topic;
		st.concat ("#");

		Serial.print("SUBSCRIBE ROUTED TOPIC: ");
		Serial.println(st.c_str());

		mqttClient->subscribe(st.c_str());
	}

	setHBLedStatus(hbLEDStatus_Running);
	m_mqttReconnectAttempAfter = 0;
	m_serverConnected = true;

	//m_SendIotBoxInfoTimeout = 15 * 60 * 1000; // 15 minutes
	//iotBoxInfo();
}
#endif


#ifdef SHP_MQTT
void ApplicationLan::onMqttMessage(const char* topic, byte* payload, unsigned int length)
{
	Serial.printf("onMqttMessage: topic='%s', payloadLen: %d\n", topic, length);

	doIncomingMessage(topic, payload, length);
}
#endif

int ApplicationLan::getDeviceCfg(uint8_t *mac, String& data)
{
	char macAddr[19];
	macAddr[18] = 0;
	sprintf(macAddr, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	#ifdef SHP_INTERNET_MODE
	String url = "https://" + m_cfgServerHostName + "/feed/iot-api/mac:" + macAddr + "/getDeviceCfg";
	#else
	String url = "http://" + m_cfgServerHostName + "/cfg/" + macHostName + ".json";
	#endif
  Serial.println("========== CFG URL1: "+url);

  WiFiClientSecure client;
	client.setInsecure();

  HTTPClient http;

  Serial.print("[HTTP] begin");
	http.setTimeout(500);
  if (http.begin(client, url))
  {
    Serial.print("[HTTP] GET...");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        data = http.getString();
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
      }
    }
    else
    {
      Serial.printf("[HTTP] Unable to connect");
    }
    http.end();

    //Serial.println(data);
		//Serial.println(data.length());

		if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
			return 1;

		return 0;
  }
}

void ApplicationLan::setup()
{
	Application::setup();

	#ifdef SHP_ETH
  	WiFi.onEvent(WiFiEvent2);
		#ifdef SHP_ETH_LAN8720
			delay(2000);
			Serial.println("### WAIT FOR ETH BEGIN 1 ### ");
			delay(2000);
			ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
		#else
			Serial.println("### WAIT FOR ETH BEGIN 2 ### ");
			delay(3000);
			ETH.begin();
		#endif
	#endif

	#ifdef SHP_WIFI
		#ifdef SHP_WIFI_MANAGER
			WiFi.begin();
			wifi_config_t wifiConfig;
			esp_err_t wifiConfigState = esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
			Serial.printf ("wifiConfigState = %d, ssid=`%s`\n", wifiConfigState, wifiConfig.sta.ssid);

			if (wifiConfig.sta.ssid[0] != 0)
			{
				Serial.println("INIT WIFI");
				WiFi.onEvent(WiFiEvent2);
			}
			else
			{
				WiFi.disconnect();

				WiFiManager wifiManager;
				//wifiManager.resetSettings();
				wifiManager.autoConnect("", "aassddffgg");
			}
		#else
			WiFi.onEvent(WiFiEvent2);
			m_wifiConnector = new ShpWiFiConnector();

			/*
			WiFi.enableIpV6();
			delay(2000);
			Serial.print("Local IPv4: ");
			Serial.println(WiFi.localIP());
			Serial.print("Local IPv6: ");
			Serial.println(WiFi.localIPv6());
			*/

			//delay(1000);
		#endif
	#endif

	#ifdef SHP_MQTT
	mqttClient = new PubSubClient();
  mqttClient->setClient(lanClient);
  mqttClient->setCallback(mqttCallback);
	#endif

	#ifdef SHP_GSM
	m_modem = new ShpModemGSM();
	#endif
}

void ApplicationLan::loadBoxConfig()
{
	if (m_cfgServerHostName.length() == 0)
	{
		app->setHBLedStatus(hbLEDStatus_Unconfigured);
		app->log(shpllError, "API server host name not set, config loading failed");
		m_loadConfigAfter = millis() + 5000;
		return;
	}

	// -- LOAD CFG FROM SERVER
	#ifdef SHP_INTERNET_MODE
	String url = "https://" + m_cfgServerHostName + "/feed/iot-api/mac:" + macHostName + "/getDeviceCfg";
	#else
	//String url = "http://" + m_cfgServerHostName + "/cfg/" + macHostName + ".json";
	String url = "https://" + m_cfgServerHostName + "/feed/iot-api/mac:" + macHostName + "/getDeviceCfg";
	#endif
  //Serial.println("========== CFG URL2: "+url);

	app->log(shpllStatus, "Loading config; URL: `%s`", url.c_str());
  String data = "";

  WiFiClientSecure client;
	client.setInsecure(); // Disable SSL certificate verification for testing purposes

  HTTPClient http;
  //Serial.print("[HTTP] begin");
	//http.setTimeout(3000);

/*
    WiFiClient* stream = http.getStreamPtr();
    while (http.connected() && (loadedBytes < m_imgDataSize))
    {
      size_t availableBytes = stream->available();
      if (availableBytes)
      {
        if (!headerLoaded)
        {
          cntBytesReaded = stream->readBytes(m_imgData, 64);
          loadedBytes += cntBytesReaded;
          parseFileHeader(m_imgData);
          Serial.printf("width: %d, height: %d \n", m_imgWidth, m_imgHeight);
          headerLoaded = true;
          continue;
        }
        size_t readedBytes = stream->readBytes(m_imgData + loadedBytes, 2048);
        if (readedBytes)
        {
          loadedBytes += readedBytes;
        }
      }
    }
*/




  if (http.begin(client, url))
  {
    //Serial.print("[HTTP] GET...");
    int httpCode = http.GET();
		delay(100);
    if (httpCode > 0)
    {
			/*****
			WiFiClient* stream = http.getStreamPtr();
			while (http.connected())
			{
				size_t availableBytes = stream->available();
				if (availableBytes)
				{
					data.concat((char)stream->read());
				}
				else
				{
					break;
				}
			}
			*****/

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        data = http.getString();
      }
      else
      {
        app->log(shpllError, "[HTTP] GET failed with status `%d`", httpCode);
      }
    }
    else
    {
      app->log(shpllError, "[HTTP] Unable to connect");
    }
    http.end();

    if (data.length())
    {
	    //app->log(shpllStatus, "Box config data length: %d; content: `%s`", data.length(), data.c_str());
			setIotBoxCfg(data);

			m_SendIotBoxInfoTimeout = 15 * 60 * 1000; // 15 minutes
			m_SendIotBoxInfoNextSend = millis() + m_SendIotBoxInfoTimeout;

			iotBoxInfo();

			return;
    }
  }

	//m_cfgServerHostName = "";
	m_loadConfigAfter = millis() + 5000;
}

boolean ApplicationLan::publish(const char *payload, const char *topic /* = NULL */)
{
	Application::publish(payload, topic);

	#ifdef SHP_MQTT
	if (eth_connected && mqttClient->state() == MQTT_CONNECTED)
	{
		boolean res = false;

		if (topic)
			res = mqttClient->publish(topic, payload, false);

		if (!res)
		{
			//Serial.println("PUBLISH FAILED!!!");
			checkMqtt();
			res = mqttClient->publish(topic, payload, false);
		}

    return res;
	}
	#else
		#ifdef SHP_INTERNET_MODE
		String url = "https://" + m_cfgServerHostName + "/feed/iot-api/mac:" + macHostName + "/setDeviceInfo";
		//Serial.println("PUBLISH1 INET MODE");
		//Serial.println("========== PUBLISH URL: "+url);
		//Serial.println(payload);
		//Serial.println("========== PUBLISH TOPIC: "+String(topic));

		WiFiClientSecure client;
		client.setInsecure();
		HTTPClient http;
		http.setReuse(false);
		http.begin(client, url);
		http.addHeader("X-IOT-TOPIC", topic);
		http.addHeader("Content-Type", "text/plain");
		int httpCode = http.POST(payload);
		http.end();
		#endif

		return true;
	#endif

	return false;
}

void ApplicationLan::publishData(uint8_t sendMode, const char *payload /* = NULL */)
{
	if (sendMode == SM_NONE)
		return;
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}

	//Serial.println("-- PUBLISH DATA LAN1 --");

	String pld;
	serializeJson(m_iotBoxInfo, pld);

	#ifdef SHP_MQTT
	if (eth_connected && mqttClient->state() == MQTT_CONNECTED)
	{
		Serial.println("-- PUBLISH DATA LAN2 --");

		boolean res = false;

		res = mqttClient->publish(m_actionTopic.c_str(), pld.c_str(), false);

		Serial.print("====== PUBLISH DATA: ");
		Serial.println(m_actionTopic.c_str());
		Serial.println(pld.c_str());
		Serial.println("--- publish done ---");

		if (!res)
		{
			checkMqtt();
			res = mqttClient->publish(m_actionTopic.c_str(), pld.c_str(), false);
		}

		Application::publishData(sendMode, pld.c_str());

		return;
	}
	#else
		Application::publishData(sendMode, pld.c_str());

		#ifdef SHP_INTERNET_MODE
		String url = "https://" + m_cfgServerHostName + "/feed/iot-api/mac:" + macHostName + "/setDeviceInfo";

		//Serial.println("PUBLISH2 INET MODE");
		//Serial.println("========== PUBLISH URL: "+url);
		//Serial.println(pld);
		//Serial.println("========== PUBLISH TOPIC: "+m_actionTopic);


		WiFiClientSecure client;
		client.setInsecure();
		HTTPClient http;
		http.setReuse(false);
		http.begin(client, url);
		http.addHeader("X-IOT-TOPIC", m_actionTopic.c_str());
		http.addHeader("Content-Type", "application/json");
		int httpCode = http.POST(pld);
		http.end();
		#endif
	#endif
}

void ApplicationLan::doFwUpgradeRequest(String payload)
{
	ShpOTAUpdate ota;
	ota.doFwUpgradeRequest(payload);
}

void ApplicationLan::loop()
{
	#ifdef SHP_WIFI
	if (m_wifiConnector)
		m_wifiConnector->loop();
	#endif

	#ifdef SHP_MQTT
  if (!mqttClient->loop())
	{
		if (m_hbLedStatus != hbLEDStatus_WaitForCfg && m_hbLedStatus != hbLEDStatus_Unconfigured)
			setHBLedStatus(hbLEDStatus_WaitForCfg);

		if (m_mqttReconnectAttempAfter && m_mqttReconnectAttempAfter < millis())
			checkMqtt();
	}
	#endif

	Application::loop();
}

void ApplicationLan::initNetworkInfo()
{
	#ifdef SHP_ETH
	ipLocal = ETH.localIP();
	macHostName = ETH.macAddress();
	#endif

	#ifdef SHP_WIFI
	ipLocal = WiFi.localIP();
	macHostName = WiFi.macAddress();
	#endif

	macHostName.replace(':', '-');
	macHostName.toLowerCase();

	// -- load servers config
	//app->log(shpllStatus, "Loading servers config from preferences");
	m_prefs.begin("ibCfgServers");
	String serversCfgData = m_prefs.getString("config", "");
	m_prefs.end();

	if (serversCfgData.length() == 0)
	{
		//app->log(shpllError, "Servers config is empty, please check your configuration");
		m_serversConfig.clear();

		return;
	}

	DeserializationError error = deserializeJson(m_serversConfig, serversCfgData.c_str());
	if (error)
	{
		app->log(shpllError, "Servers config is not valid, error: `%s`; content: `%s`", error.c_str(), serversCfgData.c_str());
		m_serversConfig.clear();
	}
	else
	{
		if (m_serversConfig.containsKey("api"))
			m_cfgServerHostName = (const char*)m_serversConfig["api"];
		else if (m_serversConfig.containsKey("http"))
			m_cfgServerHostName = (const char*)m_serversConfig["http"];
		if (m_serversConfig.containsKey("apiPort"))
		{
			m_cfgServerHostName.concat(":");
			m_cfgServerHostName.concat((const char*)m_serversConfig["apiPort"]);
		}

		if (m_serversConfig.containsKey("mqtt"))
		{
			m_mqttServerHostName = (const char*)m_serversConfig["mqtt"];
		}
		//else
		//	m_mqttServerHostName = "10.32.9.2";//(const char*)m_serversConfig["mqtt"];

		app->log(shpllStatus, "Servers config loaded, API server: `%s`, MQTT server: `%s`", m_cfgServerHostName.c_str(), m_mqttServerHostName.c_str());
	}

	m_networkInfoInitialized = true;
	m_serverConnected = true;

	m_SendIotBoxInfoTimeout = 15 * 60 * 1000; // 15 minutes

	#ifdef SHP_INTERNET_MODE
		Serial.println("INTERNET MODE");
		//iotBoxInfo();
		return;
	#endif // SHP_INTERNET_MODE

	/*
	if (m_cfgServerHostName == "")
	{
		m_cfgServerHostName = "";

		char testSrvName[64];
		struct addrinfo* result;
		int error;

		sprintf(testSrvName, "shp-iot-cfg-server-%d-%d-%d", ipLocal[0], ipLocal[1], ipLocal[2]);
		error = getaddrinfo(testSrvName, NULL, NULL, &result);
		if(error == 0)
		{
			m_cfgServerHostName = testSrvName;
			freeaddrinfo(result);
		}
		else
		{
			strcpy(testSrvName, "shp-iot-cfg-server");
			error = getaddrinfo(testSrvName, NULL, NULL, &result);
			if(error == 0)
			{
				m_cfgServerHostName = testSrvName;
				freeaddrinfo(result);
			}
			else
			{
				IPAddress cfgSrv = ipLocal;//ETH.localIP();
				cfgSrv[3] = 2;
				m_cfgServerHostName = cfgSrv.toString();
			}
		}

		Serial.print("m_cfgServerHostName: ");
		Serial.println(m_cfgServerHostName);

		m_mqttServerHostName = m_cfgServerHostName;

		m_networkInfoInitialized = true;
	}
		*/
}

void ApplicationLan::IP_Got()
{
	m_networkInfoInitialized = false;

	#ifdef SHP_MQTT
	if (mqttClient && !mqttClient->connected())
		m_mqttReconnectAttempAfter = millis() + 3000;
	#endif

	setHBLedStatus(hbLEDStatus_NetworkAddressReady);
}

void ApplicationLan::IP_Lost()
{
	setHBLedStatus(hbLEDStatus_NetworkInitialized);
}

void ApplicationLan::checkBeforeSleep()
{
	publishData(SM_NOW);

	#ifdef SHP_MQTT
  if (mqttClient)
	{
		Serial.println("mqttClient->loop");
		mqttClient->loop();
	}
	#endif

	Application::checkBeforeSleep();

	delay(200);
}

