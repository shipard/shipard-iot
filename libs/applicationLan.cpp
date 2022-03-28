#ifdef SHP_ETH
#include <ETH.h>
#endif
#include <netdb.h>
#include <lwip/dns.h>
#include <HTTPClient.h>
#include <WiFiType.h>


bool ApplicationLan::eth_connected = false;


void WiFiEvent2(system_event_id_t event)
{
  switch (event) 
  {
		#ifdef SHP_ETH
    case SYSTEM_EVENT_ETH_START:
      ETH.setHostname(app->m_deviceId.c_str());
      break;
		#endif
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
		case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi Connected");
      break;
		#ifdef SHP_ETH
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
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
		case SYSTEM_EVENT_STA_GOT_IP:
		  Serial.print("WiFi MAC: ");
      Serial.print(WiFi.macAddress());
			Serial.print(", IPv4: ");
      Serial.println(WiFi.localIP());
			ApplicationLan::eth_connected = true;
      app->IP_Got();
			break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
		case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("ETH Disconnected");
      ApplicationLan::eth_connected = false;
			app->IP_Lost();
      break;
    case SYSTEM_EVENT_ETH_STOP:
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
	//Serial.println("[MQTT] connect to server!");

	/*
	String id = (const char*)m_boxConfig["deviceId"];
	id.concat (millis());
	id.concat(rand());
	*/

	mqttClient->connect((const char*)m_boxConfig["deviceId"]/*id.c_str()*/, m_logTopic.c_str()/*"shp/iot-boxes-disconnect"*/, 0, 1, /*(const char*)m_boxConfig["deviceId"]*/"disconnect");
	
	if (!mqttClient->connected())
	{
		signalLedBlink(5);
		sleep(100);
		return;
	}

	mqttClient->subscribe(m_deviceSubTopic.c_str());

	log (shpllStatus, "device connected to MQTTT server; deviceNdx=%d, fwVersion=%s, logLevel=%d, freeMem=%ld, sdkVer=%s", (int)m_boxConfig["deviceNdx"], SHP_LIBS_VERSION, m_logLevel, ESP.getFreeHeap(), ESP.getSdkVersion());

	iotBoxInfo();
	//log (shpllStatus, "device connected to MQTTT server; deviceNdx=%d, fwVersion=%s, logLevel=%d", (int)m_boxConfig["deviceNdx"], SHP_LIBS_VERSION, m_logLevel);
}
#endif


#ifdef SHP_MQTT
void ApplicationLan::onMqttMessage(const char* topic, byte* payload, unsigned int length)
{
	char *portId = strrchr(topic, '/');

	portId++;
	if (portId == NULL || m_deviceId == portId)
	{
		return;
	}

	if (strcmp(portId, "set") == 0)
	{ 
		doSet(payload, length);
		return;
	}

	if (strcmp(portId, "get") == 0)
	{
		publishData(SM_LOOP);
		return;
	}

	// -- cmd:commandID
	if (portId[0] == 'c' && portId[1] == 'm' && portId[2] == 'd' && portId[3] == ':')
	{
		char *commandId = strrchr(portId, ':');
		if (commandId == NULL || commandId[1] == 0)
		{
			return;
		}

		commandId++;

		Serial.printf("command is `%s`\n", commandId);

		addCmdQueueItemFromMessage(commandId, payload, length);

		return;
	}

	char *subCmd = strchr(portId, ':');
	if (subCmd)
	{
		subCmd[0] = 0;
		subCmd++;
	}

	// -- ioPort event
	ShpIOPort *dstIOPort = ioPort(portId);
	if (dstIOPort == NULL)
	{
		log(shpllError, "Unknown portId `%s`", portId);
		return;
	}

	dstIOPort->onMessage(topic, subCmd, payload, length);
}
#endif


void ApplicationLan::setup()
{
	Application::setup();

	#ifdef SHP_ETH
  WiFi.onEvent(WiFiEvent2);
  ETH.begin();
	#endif

	#ifdef SHP_WIFI
	WiFi.begin();
	wifi_config_t wifiConfig;
	esp_err_t wifiConfigState = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifiConfig);
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
	http.setTimeout(5);
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
      DeserializationError error = deserializeJson(m_boxConfig, data.c_str());
      if (error) 
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        m_boxConfig.clear();
      }
			else
			{
				m_boxConfigLoaded = true;

				if (strcmp((const char*)m_boxConfig["deviceId"], m_deviceId.c_str()) != 0)
				{
					m_deviceId = (const char*)m_boxConfig["deviceId"];
					m_prefs.begin("IotBox");
					m_deviceId = m_prefs.putString("deviceId", m_deviceId);
					m_prefs.end();
				}

				mqttServerHostName = cfgServerHostName;

				m_deviceTopic = MQTT_TOPIC_THIS_DEVICE"/";
				m_deviceTopic.concat((const char*)m_boxConfig["deviceId"]);

				m_deviceSubTopic = MQTT_TOPIC_THIS_DEVICE"/";
				m_deviceSubTopic.concat((const char*)m_boxConfig["deviceId"]);
				m_deviceSubTopic.concat("/#");

				m_logTopic = MQTT_TOPIC_DEVICES_LOG_BEGIN;
				m_logTopic.concat ((const char*)m_boxConfig["deviceId"]);

				init();

				return;
			}
    }
  }
	
	cfgServerHostName = "";
	signalLedBlink(3);
	sleep(10);
}


boolean ApplicationLan::publish(const char *payload, const char *topic /* = NULL */)
{
	#ifdef SHP_MQTT
	if (eth_connected && mqttClient->connected())
	{
		boolean res = false;
		
		if (topic)
			res = mqttClient->publish(topic, payload);

		if (!res)
		{
			//Serial.println("PUBLISH FAILED!!!");
			checkMqtt();
			res = mqttClient->publish(topic, payload);
		}

    return res;
	}
	#endif
	return false;
}

void ApplicationLan::publishData(uint8_t sendMode)
{
	/*
client.beginPublish(topic, measureJson(doc), retained);
WriteBufferingPrint bufferedClient(client, 32);
serializeJson(doc, bufferedClient);
bufferedClient.flush();
client.endPublish();

	*/

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
		
		res = mqttClient->publish(m_deviceTopic.c_str(), payload.c_str());

		if (!res)
		{
			//Serial.println("PUBLISH FAILED!!!");
			checkMqtt();
			res = mqttClient->publish(m_deviceTopic.c_str(), payload.c_str());
		}

		return;
	}
	#endif
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
	signalLedOff();
}

void ApplicationLan::IP_Lost()
{
	signalLedOn();
}

