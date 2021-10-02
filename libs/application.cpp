#include <Arduino.h>

#define SHP_SERIAL_DEBUG_ON


#ifdef SHP_ETH
#include <ETH.h>
#endif

#include <netdb.h>
#include <lwip/dns.h>


#include <HTTPClient.h>
#include <WiFiType.h>




extern Application *app;

bool Application::eth_connected = false;


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
			Application::eth_connected = true;
      app->IP_Got();
      break;
			#endif
		case SYSTEM_EVENT_STA_GOT_IP:
		  Serial.print("WiFi MAC: ");
      Serial.print(WiFi.macAddress());
			Serial.print(", IPv4: ");
      Serial.println(WiFi.localIP());
			Application::eth_connected = true;
      app->IP_Got();
			break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
		case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("ETH Disconnected");
      Application::eth_connected = false;
			app->IP_Lost();
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      Application::eth_connected = false;
			app->IP_Lost();
      break;
    default:
      break;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) 
{
	app->onMqttMessage(topic, payload, length);
}


Application::Application() : 	m_logLevel(shpllStatus),
															m_pinLed(0),
															#ifdef SHP_GSM
															m_modem (NULL),
															#endif
															#ifdef SHP_MQTT
															mqttClient (NULL),
															#endif
															m_networkInfoInitialized(false), 
															m_boxConfigLoaded(false),
															m_ioPorts(NULL),
															m_countIOPorts(0),
															m_cmdQueueRequests(0)
{
  //m_pinLed = 33;

	for (int i = 0; i < APP_CMD_QUEUE_LEN; i++)
	{
		m_cmdQueue[i].qState = cqsFree;
		m_cmdQueue[i].startAfter = 0;
		m_cmdQueue[i].commandId = "";
		m_cmdQueue[i].payload = "";
	}
}

void Application::setup()
{
	#ifdef SHP_SERIAL_DEBUG_ON
  Serial.begin(115200);
	#endif

	m_prefs.begin("IotBox");
	m_deviceId = m_prefs.getString("deviceId", "unconfigured-iot-box");
	m_logLevel = m_prefs.getUChar("logLevel", shpllStatus);
	m_prefs.end();

	delay(1500); 

	if (m_pinLed)
  	pinMode(m_pinLed,OUTPUT);

  signalLedOn();

	#ifdef SHP_SERIAL_DEBUG_ON
	delay(500);
  Serial.println("fwVer: " SHP_LIBS_VERSION);
	#endif


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

	#ifdef SHP_GSM
	m_modem = new ShpModemGSM();
	#endif

	#ifdef SHP_MQTT
	mqttClient = new PubSubClient();
  mqttClient->setClient(lanClient);
  mqttClient->setCallback(mqttCallback);
	#endif
}

void Application::IP_Got()
{
	m_boxConfigLoaded = false;
	m_networkInfoInitialized = false;
	signalLedOff();
}

void Application::IP_Lost()
{
	signalLedOn();
}

boolean Application::publish(const char *payload, const char *topic /* = NULL */)
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

void Application::checks()
{
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
void Application::checkMqtt()
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

void Application::initNetworkInfo()
{
	m_networkInfoInitialized = true;
}

void Application::loadBoxConfig()
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

void Application::init()
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

	initIOPorts();
	init2IOPorts();
}

void Application::initIOPorts()
{
	JsonArray ioPorts = m_boxConfig["ioPorts"];

	m_ioPorts = new ShpIOPort*[ioPorts.size()];

	for (JsonVariant onePort: ioPorts)
	{
		const char *portType = (const char*)onePort["type"];

		if (portType == nullptr)
		{
			continue;
		}

		ShpIOPort *newPort = NULL;
		if (strcmp(portType, "controlBinary") == 0)
			newPort = new ShpControlBinary();
		#ifdef SHP_CONTROL_LEDSTRIP_H	
		else if (strcmp(portType, "controlLedStrip") == 0)
			newPort = new ShpControlLedStrip();
		#endif	
		#ifdef SHP_CONTROL_LEVEL_H	
		else if (strcmp(portType, "controlLevel") == 0)
			newPort = new ShpControlLevel();
		#endif	
		#ifdef SHP_CONTROL_HBRIDGE_H	
		else if (strcmp(portType, "controlHBridge") == 0)
			newPort = new ShpControlHBridge();
		#endif	
		else if (strcmp(portType, "inputAnalog") == 0)
			newPort = new ShpInputAnalog();
		else if (strcmp(portType, "inputBinary") == 0)
			newPort = new ShpInputBinary();
		else if (strcmp(portType, "inputCounter") == 0)
			newPort = new ShpInputCounter();
		#ifdef SHP_SENSOR_DISTANCE_US_H
		else if (strcmp(portType, "sensorDistanceUS") == 0)
			newPort = new ShpSensorDistanceUS();
		#endif	
		else if (strcmp(portType, "dataSerial") == 0)
			newPort = new ShpDataSerial();
		else if (strcmp(portType, "dataOneWire") == 0)
			newPort = new ShpDataOneWire();
		#ifdef SHP_DATA_WIEGAND_H	
		else if (strcmp(portType, "dataWiegand") == 0)
			newPort = new ShpDataWiegand();
		#endif	
		#ifdef SHP_DATA_RFIDPN532_H
		else if (strcmp(portType, "dataRFIDPN532") == 0)
			newPort = new ShpDataRFIDPN532();
		#endif	
		#ifdef SHP_MOD_RFID_1356_MIFARE_H
		else if (strcmp(portType, "dataRFIDMOD1356MIFARE") == 0)
			newPort = new ShpMODRfid1356Mifare();
		#endif	
		#ifdef SHP_DATA_RFIDPN125KHZ_H
		else if (strcmp(portType, "dataRFID125kHz") == 0)		                           
			newPort = new ShpDataRFID125KHZ();
		#endif
		#ifdef SHP_DATA_GSM_H
		else if (strcmp(portType, "dataGSM") == 0)
			newPort = new ShpDataGSM();
		#endif
		#ifdef SHP_DISPLAY_NEXTION_H
		else if (strcmp(portType, "displayNextion") == 0)
			newPort = new ShpDisplayNextion();
		#endif	
		else if (strcmp(portType, "busI2C") == 0)
			newPort = new ShpBusI2C();
		#ifdef SHP_GPIO_EXPANDER_I2C_H	
		else if (strcmp(portType, "gpioExpanderI2C") == 0)
			newPort = new ShpGpioExpanderI2C();
		#endif
		else if (strcmp(portType, "meteoDHT") == 0)
			newPort = new ShpMeteoDHT();
		else if (strcmp(portType, "meteoBME280") == 0)
			newPort = new ShpMeteoBME280();
		else if (strcmp(portType, "meteoBH1750") == 0)
			newPort = new ShpMeteoBH1750();
		
		if (newPort == NULL)
			continue;

		addIOPort(newPort, onePort);
	}
}

void Application::init2IOPorts()
{
	for (int i = 0; i < m_countIOPorts; i++)
	{
		m_ioPorts[i]->init2();
	}
}

void Application::addIOPort(ShpIOPort *ioPort, JsonVariant portCfg)
{
	//Serial.println ((const char*)portCfg["type"]);

	ioPort->init(portCfg);
	m_ioPorts[m_countIOPorts] = ioPort;
	m_countIOPorts++;
}

void Application::onMqttMessage(const char* topic, byte* payload, unsigned int length)
{
	char *portId = strrchr(topic, '/');
	if (portId == NULL)
	{
		return;
	}

	portId++;

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

void Application::addCmdQueueItemFromMessage(const char* commandId, byte* payload, unsigned int length)
{
	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	addCmdQueueItem(commandId, payloadStr, 100);
}

void Application::addCmdQueueItem(const char *commandId, String payload, unsigned long startAfter)
{
	for (int i = 0; i < APP_CMD_QUEUE_LEN; i++)
	{
		if (m_cmdQueue[i].qState != cqsFree)
			continue;

		m_cmdQueue[i].qState = cqsLocked;	
		m_cmdQueue[i].commandId = commandId;
		m_cmdQueue[i].payload = payload;
		m_cmdQueue[i].startAfter = millis() + startAfter;

		m_cmdQueue[i].qState = qsDoIt;	
		m_cmdQueueRequests++;

		break;
	}
}

ShpIOPort* Application::ioPort(const char *portId)
{
	for (int i = 0; i < m_countIOPorts; i++)
	{
		if (strcmp (m_ioPorts[i]->m_portId, portId) == 0)
			return m_ioPorts[i];
	}

	return NULL;
}

void Application::signalLedOn()
{
	if (!m_pinLed)
		return;
	digitalWrite(m_pinLed,HIGH);
	m_signalLedState = HIGH;
}

void Application::signalLedOff()
{
	if (!m_pinLed)
		return;
	digitalWrite(m_pinLed,LOW);
	m_signalLedState = LOW;
}

void Application::signalLedBlink(int count)
{
	if (!m_pinLed)
		return;

	uint8_t blinkState = (m_signalLedState == HIGH) ? LOW : HIGH; 
	for(int i = 0; i < count; i++)
	{
		digitalWrite(m_pinLed,blinkState);
		usleep(300000);
		digitalWrite(m_pinLed,m_signalLedState);
		usleep(150000);
	}
}

void Application::runCmdQueueItem(int i)
{	
	m_cmdQueueRequests--;

	if (strcmp(m_cmdQueue[i].commandId.c_str(), "fwUpgrade") == 0)
	{
		ShpOTAUpdate ota;
		ota.doFwUpgradeRequest(m_cmdQueue[i].payload);
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "reboot") == 0)
	{
		reboot();
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "logLevel") == 0)
	{
		setLogLevel(m_cmdQueue[i].payload.c_str());
	}

	m_cmdQueue[i].qState = cqsFree;
}

void Application::reboot()
{
	log(shpllStatus, "reboot");

	// -- shutdown all io ports
	for (int i = 0; i < m_countIOPorts; i++)
	{
		m_ioPorts[i]->shutdown();
	}

	// -- reboot now
	ESP.restart();
}

void Application::setLogLevel(const char *logLevel)
{
	char *err;
	long ll = strtol(logLevel, &err, 10);
	if (*err || ll < 0 || ll > shpllALL) 
	{
		log (shpllError, "invalid logLevel `%s`", logLevel);
		return;
	}

	log (shpllStatus, "logLevel changed from `%d` to `%d`", m_logLevel, ll);
	m_logLevel = ll;
	m_prefs.begin("IotBox");	
	m_prefs.putUChar("logLevel", m_logLevel);
	m_prefs.end();
}

void Application::log(uint8_t level, const char* format, ...)
{
	char msg[512];
	sprintf(msg, "[%s] ", SHP_LOG_LEVEL_NAMES[level - 1]);

	va_list argptr;
  va_start(argptr, format);
  vsnprintf(msg + strlen(msg), 512, format, argptr);
  va_end(argptr);

	log(msg, level);
}

void Application::log(const char *msg, uint8_t level)
{
	#ifdef SHP_MQTT
	if (mqttClient)
	{
		if (m_logLevel >= level)
		{
			publish(msg, m_logTopic.c_str());
			mqttClient->loop();
		}
	}
	#endif

	#ifdef SHP_SERIAL_DEBUG_ON
	Serial.println(msg);
	#endif
}

void Application::iotBoxInfo()
{
	String info;
	info.concat("{");
	info.concat("\"device\": "); info.concat((int)m_boxConfig["deviceNdx"]); info.concat(",");
	info.concat("\"type\": \"system\",");
	info.concat("\"items\":{");
		info.concat("\"device-type\": \"" SHP_DEVICE_TYPE "\",");
		info.concat("\"version-fw\": \"" SHP_LIBS_VERSION "\",");
		info.concat("\"version-os\": \""); info.concat(ESP.getSdkVersion()); info.concat("\",");
		info.concat("\"device-arch\": \""); info.concat("esp32"); info.concat("\"");
		info.concat("}");
	info.concat("}");

	publish(info.c_str(), MQTT_TOPIC_DEVICES_INFO);
}

void Application::loop()
{
	#ifdef SHP_MQTT
  mqttClient->loop();
	#endif

  this->checks();

	// -- io ports
	for (int i = 0; i < m_countIOPorts; i++)
		m_ioPorts[i]->loop();


	// -- commands queue
	if (m_cmdQueueRequests != 0)
	{
		unsigned long now = millis();

		for (int i = 0; i < APP_CMD_QUEUE_LEN; i++)
		{
			if (m_cmdQueue[i].qState != cqsDoIt)
				continue;
			if (m_cmdQueue[i].startAfter > now)
				continue;

			m_cmdQueue[i].qState = cqsRunning;
			runCmdQueueItem(i);

			break;
		}
	}
}
