#include <Arduino.h>

#define SHP_SERIAL_DEBUG_ON

#ifdef ESP32
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#endif




extern SHP_APP_CLASS *app;

RTC_DATA_ATTR int g_lastRunTimeTotal = 0;
RTC_DATA_ATTR int g_lastRunTimeRadio = 0;



Application::Application() : 	m_logLevel(shpllStatus),
															#ifdef SHP_HB_LED_MODE
															m_hbLEDMode(SHP_HB_LED_MODE),
															m_hbLEDPin(SHP_HB_LED_PIN),
															#else
															m_hbLEDMode(hbLEDMode_NONE),
															m_hbLEDPin(0),
															#endif
															m_hbLedStatus(hbLEDStatus_Initializing),
															m_hbLedStep(0),
															m_hbLedNextCheck(0),
															#ifdef SHP_GSM
															m_modem (NULL),
															#endif
															m_serverConnected(false),
															m_boxConfigLoaded(false),
															m_boxConfigInFlash(bcifUnknown),
															m_ioPorts(NULL),
															m_countIOPorts(0),
															m_routedTopicsCount(0),
															m_cmdQueueRequests(0),
															m_publishDataOnNextLoop(false),
															m_TotalLoops(0),
															m_iotBoxInfoCounter(0),
															m_useSerialComm(1),
															m_clientUART(NULL),
															m_lowPowerDevice(false),
															m_lowPowerDeviceCharging(false),
															m_enableSleepWhenCharging(false),
															m_disableAutoSleep(false),
															m_autoSleepEnabled(false),
															m_doCheckAutoSleep(false),
															m_wakeUpFromSleep(false),
															m_dsWakeupTimer(0),
															m_SendIotBoxInfoTimeout(0),
															m_SendIotBoxInfoNextSend(0)
{
	#ifdef SHP_POWER_BATT_CHARGER
	m_lowPowerDevice = true;
	#endif

	m_clientUART = new ShpClientUART();

	m_deviceNdx = 0;
	m_routedTopics = NULL;

	for (int i = 0; i < APP_CMD_QUEUE_LEN; i++)
	{
		m_cmdQueue[i].qState = cqsFree;
		m_cmdQueue[i].startAfter = 0;
		m_cmdQueue[i].commandId = "";
		m_cmdQueue[i].payload = "";
	}
}

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 4, NEO_GRB + NEO_KHZ800);

void Application::setup()
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	//#ifdef SHP_SERIAL_DEBUG_ON
  Serial.begin(115200);
	//#endif

		Serial.println("APP-SETUP1");

	// -- wakeup detect
	esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
	if (wakeup_reason)
	{
		m_wakeUpFromSleep = true;
	}

	Serial.println("APP-SETUP2");

	//m_hbLEDPin = 2; // RGB: 4
	//m_hbLEDMode = hbLEDMode_NONE;//hbLEDMode_BINARY;
	if (m_hbLEDMode == hbLEDMode_BINARY)
	{
  	pinMode(m_hbLEDPin, OUTPUT);
		digitalWrite(m_hbLEDPin, HIGH);
		m_hbLedStep = 1;
	}
	if (m_hbLEDMode == hbLEDMode_RGB)
	{
		m_hbLed = new Adafruit_NeoPixel(1, m_hbLEDPin, NEO_GRB + NEO_KHZ800);
		m_hbLed->begin();
		m_hbLed->setPixelColor(0, m_hbLed->Color(100, 0, 0));
  	m_hbLed->show();

		m_hbLedStep = 1;
	}

	//JsonObject iotBoxState = m_iotBoxInfo.createNestedObject(IOT_BOX_INFO_VALUES);

	m_prefs.begin("IotBox");
	m_deviceId = m_prefs.getString("deviceId", "unconfigured-iot-box");
	m_logLevel = m_prefs.getUChar("logLevel", shpllStatus);
	m_prefs.end();

	//delay(1500);

	#ifdef SHP_SERIAL_DEBUG_ON
	//delay(500);
  Serial.println("fwVer: " SHP_LIBS_VERSION " / " SHP_DEVICE_TYPE " / " __DATE__ " " __TIME__);
	#endif
}

boolean Application::publish(const char *payload, const char *topic /* = NULL */)
{
	if (m_useSerialComm)
	{
		Serial.print("[<<<];");
		Serial.print(topic);
		Serial.print(";");
		Serial.print(payload);
		Serial.print("\r\n");
	}

	return false;
}

void Application::publishData(uint8_t sendMode, const char *payload /* = NULL */)
{
	if (sendMode == SM_LOOP)
	{
		m_publishDataOnNextLoop = true;
		return;
	}
	if (sendMode == SM_NOW)
	{
		if (m_useSerialComm)
		{
			Serial.print("[<<<];");
			Serial.print(m_actionTopic.c_str());
			Serial.print(";");
			if (payload)
				Serial.print(payload);
			else
			{
				String p;
				serializeJson(m_iotBoxInfo, p);
				Serial.print(p);
			}
			Serial.print("\r\n");
		}
	}
}

void Application::setValue(const char *key, const char *value, uint8_t sendMode)
{
	m_iotBoxInfo[key] = value;

	publishData(sendMode);
}

void Application::setValue(const char *key, const int value, uint8_t sendMode)
{
	m_iotBoxInfo[key] = value;
	publishData(sendMode);
}

void Application::setValue(const char *key, const float value, uint8_t sendMode)
{
	m_iotBoxInfo[key] = value;
	publishData(sendMode);
}

void Application::publishAction(const char *key, const char *value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_actionTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::publishAction(const char *key, const int value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_actionTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::publishAction(const char *key, const float value)
{
	String payload;
	StaticJsonDocument<200> doc;
	doc[key] = value;
	serializeJson(doc, payload);

	publish(payload.c_str(), m_actionTopic.c_str());
	setValue(key, value, SM_NONE);
}

void Application::checks()
{
	if (m_boxConfigInFlash == bcifUnknown)
		setIotBoxFromStoredCfg();

	//Serial.println ("--checks--");
}

void Application::loadBoxConfig()
{
}

void Application::init()
{
	Serial.println("app_init_1");

	/*
	if (m_hbLEDMode == hbLEDMode_BINARY)
	{
  	pinMode(m_hbLEDPin, OUTPUT);
		digitalWrite(m_hbLEDPin, HIGH);
	}
	*/

	initIOPorts();

	init2IOPorts();

	setHBLedStatus(hbLEDStatus_Running);
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
		#ifdef SHP_CONTROL_BISTRELAY_H
		else if (strcmp(portType, "control/bist-relay") == 0)
			newPort = new ShpControlBistRelay();
		#endif
		#ifdef SHP_CONTROL_HBRIDGE_H
		else if (strcmp(portType, "control/h-bridge") == 0)
			newPort = new ShpControlHBridge();
		#endif
		#ifdef SHP_CONTROL_MATRIX_H
		else if (strcmp(portType, "control/matrix") == 0)
			newPort = new ShpControlMatrix();
		#endif
		else if (strcmp(portType, "input/analog") == 0)
			newPort = new ShpInputAnalog();
		else if (strcmp(portType, "input/capBtn") == 0)
			newPort = new ShpInputCapBtn();
		else if (strcmp(portType, "input/binary") == 0)
			newPort = new ShpInputBinary();
		else if (strcmp(portType, "input/counter") == 0)
			newPort = new ShpInputCounter();
		#ifdef SHP_SENSOR_DISTANCE_US_H
		else if (strcmp(portType, "sensorDistanceUS") == 0)
			newPort = new ShpSensorDistanceUS();
		#endif
		#ifdef SHP_SENSOR_LD2410_US_H
		else if (strcmp(portType, "sensor/ld2410") == 0)
			newPort = new ShpSensorLD2410();
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
		#ifdef SHP_EPD_DISPLAY_H
		else if (strcmp(portType, "display/epaper") == 0)
			newPort = new ShpEpdDisplay();
		#endif
		else if (strcmp(portType, "bus/i2c") == 0)
			newPort = new ShpBusI2C();
		#ifdef SHP_GPIO_EXPANDER_I2C_H
		else if (strcmp(portType, "gpio-expander/i2c") == 0)
			newPort = new ShpGpioExpanderI2C();
		#endif
		#ifdef SHP_GPIO_EXPANDER_RS485_H
		else if (strcmp(portType, "gpio-expander/rs485") == 0)
			newPort = new ShpGpioExpanderRS485();
		#endif
		#ifdef SHP_BUS_RS485_H
		else if (strcmp(portType, "bus/rs485") == 0)
			newPort = new ShpBusRS485();
		#endif
		#ifdef SHP_BUS_CAN_H
		else if (strcmp(portType, "bus/can") == 0)
			newPort = new ShpBusCAN();
		#endif
		else if (strcmp(portType, "sensor/meteo-bme280") == 0)
			newPort = new ShpMeteoBME280();
		else if (strcmp(portType, "sensor/meteo-bme68x") == 0)
			newPort = new ShpMeteoBME68x();
		else if (strcmp(portType, "sensor/meteo-bmp280") == 0)
			newPort = new ShpMeteoBMP280();
		else if (strcmp(portType, "meteoBH1750") == 0)
			newPort = new ShpMeteoBH1750();
		else if (strcmp(portType, "sensor/meteo-sht40") == 0)
			newPort = new ShpMeteoSHT40();
		#ifdef SHP_NETWORKS_ESPNOW_SERVER_H
		else if (strcmp(portType, "networks/esp-now-server") == 0)
			newPort = new ShpEspNowServerIOPort();
		#endif
		#ifdef SHP_ROUTER_CAN_H
		else if (strcmp(portType, "router/can") == 0)
			newPort = new ShpRouterCAN();
		#endif
		#ifdef SHP_POWER_BATT_USB_CHARGER_H
		else if (strcmp(portType, "power/battusb") == 0)
			newPort = new ShpPowerBattUSBCharger();
		#endif
		#ifdef SHP_CAM_ESP32
		else if (strcmp(portType, "cam/esp32") == 0)
			newPort = new ShpCamESP32();
		#endif

		if (newPort == NULL)
			continue;

		addIOPort(newPort, onePort);
	}
}

void Application::init2IOPorts()
{
	if (m_routedTopicsCount)
	{
		m_routedTopics = new RoutedTopicItem[m_routedTopicsCount];
		for (int i = 0; i < m_routedTopicsCount; i++)
		{
			m_routedTopics[i].ioPortIndex = 0;
			m_routedTopics[i].topic = NULL;
		}
	}

	m_routedTopicsCount = 0;

	for (int i = 0; i < m_countIOPorts; i++)
	{
		m_ioPorts[i]->init2();
	}
}

void Application::addIOPort(ShpIOPort *ioPort, JsonVariant portCfg)
{
	ioPort->m_appPortIndex = m_countIOPorts;
	ioPort->init(portCfg);
	m_ioPorts[m_countIOPorts] = ioPort;
	m_countIOPorts++;
}

void Application::subscribeIOPortTopic (uint8_t ioPortIndex, const char *topic)
{
	m_routedTopics[m_routedTopicsCount].ioPortIndex = ioPortIndex;
	m_routedTopics[m_routedTopicsCount].topic = topic;

	m_routedTopicsCount++;
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
		const char *value = oneItem.value().as<const char*>();
		iop->onMessage((byte*)value, strlen(value), NULL);
	}
}

void Application::doFwUpgradeRequest(String payload)
{
}

void Application::setChargingState(bool charging)
{
	if (charging)
	{
		//m_hbLEDMode = hbLEDMode_BINARY;

		m_lowPowerDeviceCharging = true;
	}
	else
	{
		//m_hbLEDMode = hbLEDMode_NONE;

		m_lowPowerDeviceCharging = false;
	}
}

void Application::doIncomingMessage(const char* topic, byte* payload, unsigned int length, uint8_t shortMode /* = 0 */)
{
	Serial.printf("doIncomingMessage: topic='%s', shortMode: %d\n", topic, shortMode);


	// -- search for route
	if (!shortMode && m_routedTopicsCount)
	{
		for (int i = 0; i < m_routedTopicsCount; i++)
		{
			Serial.print("check routed topic: ");
			Serial.println(m_routedTopics[i].topic);

			if (strncmp(m_routedTopics[i].topic, topic, strlen(m_routedTopics[i].topic)) != 0)
				continue;
			Serial.printf("ROUTE: %s\n", m_routedTopics[i].topic);
			m_ioPorts[m_routedTopics[i].ioPortIndex]->routeMessage(topic, payload, length);
			return;
		}
	}

	if (!shortMode && strncmp(topic, m_deviceTopic.c_str(), m_deviceTopic.length()))
		return;

	const char *portId;

	if (!shortMode)
		portId = topic + m_deviceTopic.length();
	else
		portId = topic;

	/*
	char *subCmd = strchr(portId, '/');
	if (subCmd)
	{
		//subCmd[0] = 0;
		//subCmd++;
	}
	*/
	//Serial.printf("doIncomingMessage: portId1='%s'\n", portId);

	if (portId == NULL || portId[0] == 0)
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
	if (portId && portId[0] == 'c' && portId[1] == 'm' && portId[2] == 'd' && portId[3] == ':')
	{
		char *commandId = strrchr(portId, ':');
		if (commandId == NULL || commandId[1] == 0)
		{
			return;
		}

		commandId++;

		//Serial.printf("command is `%s`\n", commandId);

		addCmdQueueItemFromMessage(commandId, payload, length);

		return;
	}


	// -- ioPort event
	ShpIOPort *dstIOPort = ioPort(portId);
	if (dstIOPort != NULL)
	{
		dstIOPort->onMessage(payload, length, topic);
		return;
	}


	log(shpllError, "Unknown portId `%s`", portId);
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

void Application::doHBLed()
{
	if (m_hbLEDMode == hbLEDMode_NONE)
		return;

	static uint16_t hbLedTimingsBinary [6][hbLEDMode_BINARY_STEPS] = {
		100, 100, // hbLEDStatus_Initializing
		300, 100, // hbLEDStatus_NetworkInitialized
		600, 100, // hbLEDStatus_NetworkAddressReady
		900, 100, // hbLEDStatus_WaitForCfg
		1000, 250, // hbLEDStatus_Running
		50, 50  // hbLEDStatus_Unconfigured
	};

	if (m_hbLEDMode == hbLEDMode_BINARY)
	{
		m_hbLedStep = m_hbLedStep ? 0 : 1;
		m_hbLedNextCheck = millis() + hbLedTimingsBinary[m_hbLedStatus][m_hbLedStep];
		//printf("doHBLed, status is %d, step is %d, nextStatusTime is %d, pinState: %d\n", m_hbLedStatus, m_hbLedStep, hbLedTimingsBinary[m_hbLedStatus][m_hbLedStep], m_hbLedStep ? LOW : HIGH);
		digitalWrite(m_hbLEDPin, m_hbLedStep);
		return;
	}

	static uint16_t hbLedTimingsRGB [6][hbLEDMode_RGB_STEPS] = {
		100, 100, // hbLEDStatus_Initializing
		300, 100, // hbLEDStatus_NetworkInitialized
		600, 100, // hbLEDStatus_NetworkAddressReady
		900, 100, // hbLEDStatus_WaitForCfg
		1800, 200, // hbLEDStatus_Running
		200, 75  // hbLEDStatus_Unconfigured
	};

	static uint32_t hbLedColorsRGB [6][hbLEDMode_RGB_STEPS] = {
		0x0000a0, 0xa00000, // hbLEDStatus_Initializing
		0x000000, 0xffa500, // hbLEDStatus_NetworkInitialized
		0x000000, 0xfff700, // hbLEDStatus_NetworkAddressReady
		0x000000, 0x7f00ff, // hbLEDStatus_WaitForCfg
		0x000000, 0x001800, // hbLEDStatus_Running
		0x505050, 0xff0000  // hbLEDStatus_Unconfigured
	};


	if (m_hbLEDMode == hbLEDMode_RGB)
	{
		m_hbLedStep = m_hbLedStep ? 0 : 1;
		m_hbLedNextCheck = millis() + hbLedTimingsRGB[m_hbLedStatus][m_hbLedStep];
		m_hbLed->setPixelColor(0, hbLedColorsRGB[m_hbLedStatus][m_hbLedStep]);
		m_hbLed->show();
	}
}

void Application::setDSWakeupTimer(int seconds)
{
	m_dsWakeupTimer = seconds;
}

void Application::setHBLedStatus(uint8_t status)
{
	app->log(shpllStatus, "HB LED status is `%d`", status);

	m_hbLedStatus = status;
	m_hbLedNextCheck = 0;
}

void Application::runCmdQueueItem(int i)
{
	m_cmdQueueRequests--;

	if (strcmp(m_cmdQueue[i].commandId.c_str(), "fwUpgrade") == 0)
	{
		doFwUpgradeRequest(m_cmdQueue[i].payload);
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "reboot") == 0)
	{
		doReboot();
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "info") == 0)
	{
		iotBoxInfo();
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "getCfg") == 0)
	{
		getIotBoxCfg();
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "setCfg") == 0)
	{
		Serial.println("setCfg1");
		saveIotBoxCfg(m_cmdQueue[i].payload, true);
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "setCfgWiFi") == 0)
	{
		saveIotBoxCfgWiFi(m_cmdQueue[i].payload, true);
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "setCfgServers") == 0)
	{
		saveIotBoxCfgServers(m_cmdQueue[i].payload, true);
	}
	else if (strcmp(m_cmdQueue[i].commandId.c_str(), "logLevel") == 0)
	{
		setLogLevel(m_cmdQueue[i].payload.c_str());
	}

	m_cmdQueue[i].qState = cqsFree;
}

void Application::doReboot()
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

void Application::doSleep()
{
	checkBeforeSleep();

	//log(shpllStatus, "sleep");

	#ifdef SHP_WIFI
	esp_wifi_stop();
	#endif

	// -- shutdown all io ports
	for (int i = 0; i < m_countIOPorts; i++)
	{
		m_ioPorts[i]->shutdown();
	}

	if (m_dsWakeupTimer)
	{
		Serial.printf("=== Set deep sleep wakeup timer to %d secs ===\n", m_dsWakeupTimer);
		esp_sleep_enable_timer_wakeup(m_dsWakeupTimer * 1000000ULL);
	}

  g_lastRunTimeTotal = millis();

	esp_deep_sleep_start();
}

void Application::checkBeforeSleep()
{

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
	if (m_logLevel >= level)
	{
		publish(msg, m_logTopic.c_str());
	}
	else
	{
		#ifdef SHP_SERIAL_DEBUG_ON
		Serial.println(msg);
		#endif
	}
}

void Application::iotBoxInfo()
{
	uint8_t bootMode = 0; // no boot
	if (!m_iotBoxInfoCounter)
	{
		if (m_wakeUpFromSleep)
			bootMode = 2;
		else
			bootMode = 1;
	}
	long uptime = millis() / 1000;
	String info;
	info.concat("{");
	info.concat("\"devNdx\": "); info.concat((int)m_boxConfig["deviceNdx"]); info.concat(",");
	info.concat("\"devId\": \"" + m_deviceId + "\",");
	info.concat("\"boot\":"); info.concat(bootMode); info.concat(",");
	info.concat("\"ibcnt\":"); info.concat(m_iotBoxInfoCounter); info.concat(",");

	if (g_lastRunTimeTotal)
	{
		info.concat("\"lastRunTime\":"); info.concat(g_lastRunTimeTotal); info.concat(",");
		g_lastRunTimeTotal = 0;
	}

	#ifdef SHP_WIFI
	info.concat("\"macW\": \"" + WiFi.macAddress() + "\",");
	info.concat("\"rssiW\": " + String(WiFi.RSSI()) + ",");
	info.concat("\"ssidW\": \"" + String(WiFi.SSID()) + "\",");
	#endif
	info.concat("\"type\": \"system\",");
	info.concat("\"uptime\": "); info.concat(uptime); info.concat(",");
	info.concat("\"ram\": "); info.concat(ESP.getHeapSize()); info.concat(",");
	info.concat("\"freeMem\": "); info.concat(ESP.getFreeHeap()); info.concat(",");
	info.concat("\"psram\": "); info.concat(ESP.getPsramSize()); info.concat(",");
	info.concat("\"logLevel\": "); info.concat(m_logLevel); info.concat(",");
	info.concat("\"items\":{");
		info.concat("\"devType\": \"" SHP_DEVICE_TYPE "\",");
		info.concat("\"verFW\": \"" SHP_LIBS_VERSION "\",");
		info.concat("\"verOS\": \""); info.concat(ESP.getSdkVersion()); info.concat("\",");

		info.concat("\"verAL\": \""); info.concat(ESP_ARDUINO_VERSION_MAJOR); info.concat(".");
		info.concat(ESP_ARDUINO_VERSION_MINOR); info.concat(".");
		info.concat(ESP_ARDUINO_VERSION_PATCH); info.concat("\",");

		info.concat("\"arch\": \""); info.concat("esp32"); info.concat("\"");
		info.concat("}");
	info.concat("}");

	publish(info.c_str(), MQTT_TOPIC_DEVICES_INFO);

	m_iotBoxInfoCounter++;
}

void Application::getIotBoxCfg()
{
	String cfg;
	serializeJson(m_boxConfig, cfg);
	publish(cfg.c_str(), MQTT_TOPIC_DEVICES_CFG);
}

void Application::saveIotBoxCfg(String cfg, bool rebootNow /* = false */)
{
	Serial.println("setCfg2");

	m_prefs.begin("IotBox");
	m_prefs.putString("config", cfg);
	m_prefs.end();

	if (rebootNow)
		doReboot();
}

void Application::setIotBoxCfg(String data)
{
	Serial.println("### Application::setIotBoxCfg ### ");
	Serial.println(data);

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
		m_deviceNdx = m_boxConfig["deviceNdx"];

		if (strcmp((const char*)m_boxConfig["deviceId"], m_deviceId.c_str()) != 0)
		{
			m_deviceId = (const char*)m_boxConfig["deviceId"];
			m_prefs.begin("IotBox");
			m_deviceId = m_prefs.putString("deviceId", m_deviceId);
			m_prefs.end();
		}

		if (m_boxConfig.containsKey("dsWakeupTimer"))
		{
			int dswt = m_boxConfig["dsWakeupTimer"];
			m_dsWakeupTimer = dswt;
			Serial.print("dsWakeupTimer: ");
			Serial.println(m_dsWakeupTimer);

			m_autoSleepEnabled = true;
			m_lowPowerDevice = true;
		}

		/*
		if (m_boxConfig.containsKey("hbLEDMode"))
			m_hbLEDMode = m_boxConfig["hbLEDMode"];
		if (m_boxConfig.containsKey("hbLEDPin"))
			m_hbLEDPin = m_boxConfig["hbLEDPin"];
		*/

		m_deviceTopic = MQTT_TOPIC_THIS_DEVICE"/";
		m_deviceTopic.concat((const char*)m_boxConfig["deviceId"]);
		m_deviceTopic.concat("/");

		m_actionTopic = MQTT_TOPIC_THIS_DEVICE"/";
		m_actionTopic.concat((const char*)m_boxConfig["deviceId"]);

		//m_deviceSubTopic = MQTT_TOPIC_THIS_DEVICE"/";
		//m_deviceSubTopic.concat((const char*)m_boxConfig["deviceId"]);
		//m_deviceSubTopic.concat("/#");

		m_logTopic = MQTT_TOPIC_DEVICES_LOG_BEGIN;
		m_logTopic.concat ((const char*)m_boxConfig["deviceId"]);

		init();

		return;
	}
}

bool Application::setIotBoxFromStoredCfg()
{
	return false;

	//Serial.println("### Application::setIotBoxFromStoredCfg ### ");
	app->m_prefs.begin("IotBox");
	String data = app->m_prefs.getString("config", "");
	app->m_prefs.end();

	if (data.length() != 0)
	{
		Serial.println("### Application::setIotBoxFromStoredCfg ### ");
		Serial.println(data);
		m_boxConfigInFlash = bcifStoredInFlash;
		setIotBoxCfg(data);
		return true;
	}

	m_boxConfigInFlash = bcifNotStoredInFlash;
	return false;
}

void Application::saveIotBoxCfgWiFi(String cfg, bool rebootNow /* = false */)
{
	/*
	[
    {
        "ssid": "mySSID1",
        "password": "myPassword1"
    },
    {
        "ssid": "mySSID2",
        "password": "myPassword2"
    }
	]
	*/
	Serial.println("setCfgWiFi");

	m_prefs.begin("ibCfgWiFi");
	m_prefs.putString("config", cfg);
	m_prefs.end();

	if (rebootNow)
		doReboot();
}

void Application::saveIotBoxCfgServers(String cfg, bool rebootNow /* = false */)
{
	/*
		{
			"http": "server.example.com", "httpPort": 32123,
			"mqtt": "mqtt.example.com", "mqttPort": 1883
		}
	*/
	Serial.println("setCfgServers");

	m_prefs.begin("ibCfgServers");
	m_prefs.putString("config", cfg);
	m_prefs.end();

	if (rebootNow)
		doReboot();
}

void Application::checkAutoSleep()
{
	doSleep();
}

void Application::loop()
{
  this->checks();

	if (m_useSerialComm)
	{
		m_clientUART->loop();
	}

	unsigned long now = millis();

	if (m_lowPowerDevice && !m_doCheckAutoSleep && now > 5 * 60 * 1000)
	{
		Serial.printf("=== EMERGENCY SLEEP FOR INACTIVITY: %d ms after boot ===\n", now);
		setDSWakeupTimer(10 * 60); // 10 minutes
		m_doCheckAutoSleep = true;
		doSleep();
		return;
	}

  if (!app->m_serverConnected)
	{
		//Serial.println("Server not connected, skip loop");
		//return;
	}


	// -- io ports
	for (int i = 0; i < m_countIOPorts; i++)
		m_ioPorts[i]->loop();

	// -- commands queue
	if (m_cmdQueueRequests != 0)
	{
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

	if (m_hbLedNextCheck < now)
		doHBLed();

	if (m_SendIotBoxInfoTimeout)
	{
		if (m_SendIotBoxInfoNextSend < now)
		{
			iotBoxInfo();
			m_SendIotBoxInfoNextSend = now + m_SendIotBoxInfoTimeout;
		}
	}

	if (m_autoSleepEnabled && m_doCheckAutoSleep && m_TotalLoops > 10)
	{
		if (!m_lowPowerDeviceCharging || m_enableSleepWhenCharging)
			checkAutoSleep();
	}

	m_TotalLoops++;
}
