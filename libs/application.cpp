#include <Arduino.h>

#define SHP_SERIAL_DEBUG_ON





extern SHP_APP_CLASS *app;


Application::Application() : 	m_logLevel(shpllStatus),
															m_pinLed(0),
															#ifdef SHP_GSM
															m_modem (NULL),
															#endif
															m_boxConfigLoaded(false),
															m_ioPorts(NULL),
															m_countIOPorts(0),
															m_cmdQueueRequests(0),
															m_publishDataOnNextLoop(false)
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

	JsonObject iotBoxState = m_iotBoxInfo.createNestedObject(IOT_BOX_INFO_VALUES);

	m_prefs.begin("IotBox");
	m_deviceId = m_prefs.getString("deviceId", "unconfigured-iot-box");
	m_logLevel = m_prefs.getUChar("logLevel", shpllStatus);
	m_prefs.end();

	//delay(1500); 

	if (m_pinLed)
  	pinMode(m_pinLed, OUTPUT);

  signalLedOn();

	#ifdef SHP_SERIAL_DEBUG_ON
	//delay(500);
  Serial.println("fwVer: " SHP_LIBS_VERSION " / " SHP_DEVICE_TYPE " / " __DATE__ " " __TIME__);
	#endif
}

boolean Application::publish(const char *payload, const char *topic /* = NULL */)
{
	return false;
}

void Application::publishData(uint8_t sendMode)
{
}

void Application::setValue(const char *key, const char *value, uint8_t sendMode)
{
	JsonObject iotBoxState = m_iotBoxInfo[IOT_BOX_INFO_VALUES];
	iotBoxState[key] = value;
	publishData(sendMode);
}

void Application::setValue(const char *key, const int value, uint8_t sendMode)
{
	JsonObject iotBoxState = m_iotBoxInfo[IOT_BOX_INFO_VALUES];
	iotBoxState[key] = value;
	publishData(sendMode);
}

void Application::setValue(const char *key, const float value, uint8_t sendMode)
{
	JsonObject iotBoxState = m_iotBoxInfo[IOT_BOX_INFO_VALUES];
	iotBoxState[key] = value;
	publishData(sendMode);
}

void Application::publishAction(const char *key, const char *value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_deviceTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::publishAction(const char *key, const int value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_deviceTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::publishAction(const char *key, const float value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_deviceTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::checks()
{
}

void Application::loadBoxConfig()
{
}

void Application::init()
{
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
		if (strcmp(portType, "control/binary") == 0)
			newPort = new ShpControlBinary();
		#ifdef SHP_CONTROL_LEDSTRIP_H	
		else if (strcmp(portType, "control/led-strip") == 0)
			newPort = new ShpControlLedStrip();
		#endif	
		#ifdef SHP_CONTROL_LEVEL_H	
		else if (strcmp(portType, "control/level") == 0)
			newPort = new ShpControlLevel();
		#endif	
		#ifdef SHP_CONTROL_HBRIDGE_H	
		else if (strcmp(portType, "controlHBridge") == 0)
			newPort = new ShpControlHBridge();
		#endif
		else if (strcmp(portType, "input/analog") == 0)
			newPort = new ShpInputAnalog();
		else if (strcmp(portType, "input/binary") == 0)
			newPort = new ShpInputBinary();
		else if (strcmp(portType, "input/counter") == 0)
			newPort = new ShpInputCounter();
		#ifdef SHP_SENSOR_DISTANCE_US_H
		else if (strcmp(portType, "sensorDistanceUS") == 0)
			newPort = new ShpSensorDistanceUS();
		#endif	
		else if (strcmp(portType, "data/serial") == 0)
			newPort = new ShpDataSerial();
		else if (strcmp(portType, "bus/1wire") == 0)
			newPort = new ShpDataOneWire();
		#ifdef SHP_DATA_WIEGAND_H	
		else if (strcmp(portType, "data/wiegand") == 0)
			newPort = new ShpDataWiegand();
		#endif	
		#ifdef SHP_DATA_RFIDPN532_H
		else if (strcmp(portType, "rfid/pn532") == 0)
			newPort = new ShpDataRFIDPN532();
		#endif	
		#ifdef SHP_MOD_RFID_1356_MIFARE_H
		else if (strcmp(portType, "rfid/oli-mod1356-mif") == 0)
			newPort = new ShpMODRfid1356Mifare();
		#endif	
		#ifdef SHP_DATA_RFIDPN125KHZ_H
		else if (strcmp(portType, "rfid/125kHz") == 0)		                           
			newPort = new ShpDataRFID125KHZ();
		#endif
		#ifdef SHP_DATA_GSM_H
		else if (strcmp(portType, "signal/gsm") == 0)
			newPort = new ShpDataGSM();
		#endif
		#ifdef SHP_DISPLAY_NEXTION_H
		else if (strcmp(portType, "display/nextion") == 0)
			newPort = new ShpDisplayNextion();
		#endif	
		else if (strcmp(portType, "bus/i2c") == 0)
			newPort = new ShpBusI2C();
		#ifdef SHP_GPIO_EXPANDER_I2C_H
		else if (strcmp(portType, "gpio-expander/i2c") == 0)
			newPort = new ShpGpioExpanderI2C();
		#endif
		#ifdef SHP_METEO_DHT_US_H
		else if (strcmp(portType, "meteoDHT") == 0)
			newPort = new ShpMeteoDHT();
		#endif	
		else if (strcmp(portType, "sensor/meteo-bme280") == 0)
			newPort = new ShpMeteoBME280();
		else if (strcmp(portType, "meteoBH1750") == 0)
			newPort = new ShpMeteoBH1750();
		#ifdef SHP_NETWORKS_ESPNOW_SERVER_H
		else if (strcmp(portType, "networks/esp-now-server") == 0)
			newPort = new ShpEspNowServerIOPort();
		#endif

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
	ioPort->init(portCfg);
	m_ioPorts[m_countIOPorts] = ioPort;
	m_countIOPorts++;
}

void Application::doSet(byte* payload, unsigned int length)
{
	StaticJsonDocument<4096> data;
	DeserializationError error = deserializeJson(data, payload, length);
	if (error) 
	{
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.c_str());
		return;
	}
	JsonObject documentRoot = data.as<JsonObject>();
	//JsonObject setItems = data["set"];
	//for (JsonPair oneItem: setItems)
	for (JsonPair oneItem: documentRoot)
	{
		ShpIOPort* iop = ioPort(oneItem.key().c_str());
		if (!iop)
		{
			continue;
		}
		const char *value = oneItem.value().as<char*>();
		iop->onMessage("", "", (byte*)value, strlen(value));
	}	
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
		#ifdef SHP_OTA_UPDATE_H
		ShpOTAUpdate ota;
		ota.doFwUpgradeRequest(m_cmdQueue[i].payload);
		#endif
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
	/*
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
	*/

	if (m_logLevel >= level)
	{
		publish(msg, m_logTopic.c_str());
	}

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
		//info.concat("\"device-id\": \"" + m_deviceId + "\",");
		info.concat("\"device-type\": \"" SHP_DEVICE_TYPE "\",");
		info.concat("\"version-fw\": \"" SHP_LIBS_VERSION "\",");
		info.concat("\"version-os\": \""); info.concat(ESP.getSdkVersion()); info.concat("\",");
		info.concat("\"device-arch\": \""); info.concat("esp32"); info.concat("\"");
		info.concat("}");
	info.concat("}");

	publish(info.c_str(), MQTT_TOPIC_DEVICES_INFO);
}

void Application::setIotBoxCfg(String data)
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

void Application::loop()
{
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

	if (m_publishDataOnNextLoop)
	{
		m_publishDataOnNextLoop = false;
		publishData(SM_NOW);
	}
}
