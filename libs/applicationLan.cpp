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


bool ApplicationLan::eth_connected = false;


void WiFiEvent2(WiFiEvent_t event)
{
  switch (event)
  {
		#ifdef SHP_ETH
    case ARDUINO_EVENT_ETH_START:
			Serial.println("ETH Started");
      ETH.setHostname(app->m_deviceId.c_str());
      break;
		#endif
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi Connected");
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
																		m_networkInfoInitialized(false)
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
		return;

	if (!m_networkInfoInitialized)
	{
		initNetworkInfo();
		return;
	}

	if (!m_boxConfigLoaded)
	{
		loadBoxConfig();
		return;
	}

	#ifdef SHP_MQTT
	checkMqtt();
	#endif
}

#ifdef SHP_MQTT
void ApplicationLan::checkMqtt()
{
  if (!mqttClient || mqttClient->connected())
		return;

	mqttClient->setServer(mqttServerHostName.c_str(), 1883);
	Serial.println("[MQTT] connect to server!");

	/*
	String id = (const char*)m_boxConfig["deviceId"];
	id.concat (millis());
	id.concat(rand());
	*/

	mqttClient->connect((const char*)m_boxConfig["deviceId"], m_logTopic.c_str(), 0, 0, "disconnect");

	if (!mqttClient->connected())
	{
		setHBLedStatus(hbLEDStatus_WaitForCfg);
		sleep(100);
		return;
	}

	String dst = m_deviceTopic + "#";
	mqttClient->subscribe(dst.c_str());
	Serial.printf("subscribe device topic: `%s`\n", dst.c_str());

	Serial.printf("[MQTT] subscribe routed topics: %d\n", m_routedTopicsCount);
	for (int i = 0; i < m_routedTopicsCount; i++)
	{
		Serial.print("SUBSCRIBE ROUTED TOPIC: ");
		Serial.println(m_routedTopics[i].topic);

		String st = m_routedTopics[i].topic;
		st.concat ("/#");
		mqttClient->subscribe(st.c_str());
	}

	iotBoxInfo();
}
#endif


//#ifdef SHP_MQTT
void ApplicationLan::onMqttMessage(const char* topic, byte* payload, unsigned int length)
{
	doIncomingMessage(topic, payload, length);
}
//#endif

int ApplicationLan::getDeviceCfg(uint8_t *mac, String& data)
{
	char macAddr[19];
	macAddr[18] = 0;
	sprintf(macAddr, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  String url = "http://" + cfgServerHostName + "/cfg/" + macAddr + ".json";

  Serial.println("========== CFG URL: "+url);

  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin");
	http.setTimeout(1000);
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
			ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
		#else
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
		//wifiManager.resetSettings();
		WiFi.disconnect();

		WiFiManager wifiManager;
		wifiManager.autoConnect("", "aassddffgg");
	}
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
	// -- PREPARE CFG SERVER HOST NAME
	if (cfgServerHostName == "")
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

		cfgServerHostName = "";

		char testSrvName[64];
		struct addrinfo* result;
		int error;

		sprintf(testSrvName, "shp-iot-cfg-server-%d-%d-%d", ipLocal[0], ipLocal[1], ipLocal[2]);
		error = getaddrinfo(testSrvName, NULL, NULL, &result);
		if(error == 0)
		{
			cfgServerHostName = testSrvName;
			freeaddrinfo(result);
		}
		else
		{
			strcpy(testSrvName, "shp-iot-cfg-server");
			error = getaddrinfo(testSrvName, NULL, NULL, &result);
			if(error == 0)
			{
				cfgServerHostName = testSrvName;
				freeaddrinfo(result);
			}
			else
			{
				IPAddress cfgSrv = ipLocal;//ETH.localIP();
				cfgSrv[3] = 2;
				cfgServerHostName = cfgSrv.toString();
			}
		}

		Serial.print("cfgServerHostName: ");
		Serial.println(cfgServerHostName);
	}

	// -- LOAD CFG FROM SERVER
  String url = "http://" + cfgServerHostName + "/cfg/" + macHostName + ".json";
  Serial.println("========== CFG URL: "+url);

  String data;

  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin");
	http.setTimeout(5000);
  if (http.begin(client, url))
  {
    Serial.print("[HTTP] GET...");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.printf("[HTTP] GET... code: %d", httpCode);

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

    Serial.println(data);

    if (data.length())
    {
			setIotBoxCfg(data);
			return;
    }
  }

	cfgServerHostName = "";
	sleep(10);
}


boolean ApplicationLan::publish(const char *payload, const char *topic /* = NULL */)
{
	#ifdef SHP_MQTT
	if (eth_connected && mqttClient->connected())
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
	#endif
	return false;
}

void ApplicationLan::publishData(uint8_t sendMode)
{
	if (sendMode == SM_NONE)
		return;
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}

	String payload;
	serializeJson(m_iotBoxInfo, payload);

	#ifdef SHP_MQTT
	if (eth_connected && mqttClient->connected())
	{
		boolean res = false;

		res = mqttClient->publish(m_actionTopic.c_str(), payload.c_str(), false);

		if (!res)
		{
			//Serial.println("PUBLISH FAILED!!!");
			checkMqtt();
			res = mqttClient->publish(m_actionTopic.c_str(), payload.c_str(), false);
		}

		return;
	}
	#endif
}

void ApplicationLan::doFwUpgradeRequest(String payload)
{
	ShpOTAUpdate ota;
	ota.doFwUpgradeRequest(payload);
}

void ApplicationLan::loop()
{
	#ifdef SHP_MQTT
  mqttClient->loop();
	#endif

	Application::loop();
}

void ApplicationLan::initNetworkInfo()
{
	m_networkInfoInitialized = true;
}

void ApplicationLan::IP_Got()
{
	m_boxConfigLoaded = false;
	m_networkInfoInitialized = false;
	setHBLedStatus(hbLEDStatus_NetworkAddressReady);
}

void ApplicationLan::IP_Lost()
{
	//signalLedOn();
}

