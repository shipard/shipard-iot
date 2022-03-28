extern SHP_APP_CLASS *app;


extern int g_cntUarts;


ShpDataGSM::ShpDataGSM() : 
														m_speed(-1),
														m_mode(0),
														m_rxPin(-1), 
														m_txPin(-1),
														m_incomingCallMaxRings(5),
														m_outgoingCallMaxSecs(60),
														m_checkInterval(1000),
														m_nextCheck(0),
														m_checkModuleInterval(60*1000),
														m_nextModuleCheck(0),
														m_hwSerial(NULL),
														m_moduleInitialized(false),
														m_incomingCallRingInProgress(0),
														m_outgoingCallInProgress(false)
{
	for (int i = 0; i < DATA_GSM_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].action = 0;
		m_queue[i].startAfter = 0;
		m_queue[i].phoneNumber[0] = 0;
	}
}

void ShpDataGSM::init(JsonVariant portCfg)
{
	/* serial port config format:
	 * --------------------------
	 {
			"type": "dataSerial",
			"portId": "uio-5-1",
			"speed": "5",
			"pinRX": 16,
			"pinTX": 17,
			"mode": "1"
		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	m_incomingCallPhoneNumber[0] = 0;

	// -- port speed
	int speed = -1;
	if (portCfg["speed"] != nullptr)
		speed = portCfg["speed"];
	if (speed < 0 || speed >= SERIAL_SPEED_MAP_CNT)	
		return;
	m_speed = SERIAL_SPEED_MAP[speed];

	// -- port mode (bits, parity, stop bits)
	int mode = -1;
	if (portCfg["mode"] != nullptr)
		mode = portCfg["mode"];
	if (mode < 0 || mode >= SERIAL_MODE_MAP_CNT)	
		return;
	m_mode = SERIAL_MODE_MAP[mode];

	// -- rx / tx pin
	if (portCfg["pinRX"] != nullptr)
		m_rxPin = portCfg["pinRX"];
	if (portCfg["pinTX"] != nullptr)
		m_txPin = portCfg["pinTX"];

	if (m_txPin < 0 || m_rxPin < 0 || m_speed < 1 || m_mode < 1)
	{
		log (shpllError, "invalid configuration");
		return;
	}

	
	log(shpllDebug, "configured: pinRX=%d, pinTX=%d, speed=%d, mode=%x", m_rxPin, m_txPin, m_speed, m_mode);

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(m_speed, m_mode, m_rxPin, m_txPin);
}

void ShpDataGSM::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	if (!subCmd)
	{
		return;
	}

	uint8_t action = dgaInvalid;
	if (strcmp(subCmd, "sms") == 0)
		action = dgaSms;
	else if (strcmp(subCmd, "call") == 0)
		action = dgaCall;
	else if (strcmp(subCmd, "hangup") == 0)
	{
		hangup();
		return;
	}

	if (action == dgaInvalid)
	{
		log (shpllError, "invalid command `%s`", subCmd);
		return;
	}

	char phoneNumber[MAX_PHONE_NUMBER_LEN];
	int pnp = 0;
	while (pnp < length)
	{
		if (payload[pnp] == ' ')
			break;
		phoneNumber[pnp] = payload[pnp];
		phoneNumber[pnp + 1] = 0;

		pnp++;
		if (pnp >= MAX_PHONE_NUMBER_LEN)
		{
			log (shpllError, "invalid or missing phone number");
			return;
		}
	}

	String payloadStr;

	for (int i = pnp + 1; i < length; i++)
		payloadStr.concat((char)payload[i]);
	
	if (phoneNumber[0] == 0)
	{
		log (shpllError, "invalid or missing phone number");
		return;
	}

	addQueueItem(action, phoneNumber, payloadStr);
}

void ShpDataGSM::addQueueItem(int8_t action, const char *phoneNumber, String text)
{
	for (int i = 0; i < DATA_GSM_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;	
		strcpy(m_queue[i].phoneNumber, phoneNumber);
		m_queue[i].action = action;
		m_queue[i].text = text;
		m_queue[i].startAfter = millis() + 10;

		m_queue[i].qState = qsDoIt;	
		m_queueRequests++;

		break;
	}
}

void ShpDataGSM::runQueueItem(int i)
{	
	m_queueRequests--;

	if (m_queue[i].action == dgaSms)
	{
		sendSMS(m_queue[i].phoneNumber, m_queue[i].text.c_str());
	}
	else if (m_queue[i].action == dgaCall)
	{
		call(m_queue[i].phoneNumber);
	}

	m_queue[i].qState = qsFree;
}

bool ShpDataGSM::sendCmd(const char* cmd, const char *expectedResult /* = NULL */)
{
	m_responseRows = 0;
	m_response[0][0] = 0;

	//Serial.println("before send command");
	size_t res = m_hwSerial->println(cmd);
	//Serial.printf("after send command: %d\n", res);

	if (expectedResult)
	{
		delay(200);
		readResponse();
		if (m_responseRows < 2)
			return false;

		if (strcmp(m_response[m_responseRows - 1], expectedResult) == 0)
			return true;

		if (strcmp(m_response[m_responseRows - 1], "OK") == 0 && m_responseRows > 3 && strcmp(m_response[m_responseRows - 3], expectedResult) == 0)
			return true;

		if (strcmp(m_response[m_responseRows - 1], "OK") == 0 && m_responseRows > 2 && strcmp(m_response[m_responseRows - 2], expectedResult) == 0)
			return true;

		return false;	
	}

	return true;
}

bool ShpDataGSM::readResponse()
{
	bool waitForAll = true;

	m_responseCharPosition = 0;
	m_responseRows = 0;
	m_response[0][0] = 0;
	int responseBytes = 0;
	int totalResponseBytes = 0;

	unsigned long now = millis();
	unsigned long timeout = 3000; // 3 secs

	while (!responseBytes)
	{
		while (m_hwSerial->available()) 
		{
			char c = (char)m_hwSerial->read();
			responseBytes++;
			totalResponseBytes++;
			now = millis();
			if (c == 13)
				continue; // \r
			if (c == 10)
			{ // \n
				m_responseRows++;
				m_responseCharPosition = 0;
				m_response[m_responseRows][m_responseCharPosition] = 0;

				if (m_responseRows >= 2 && m_response[m_responseRows - 1][0] == 0 && strcmp(m_response[m_responseRows - 2], "OK") == 0)
					break;

				continue;
			}

			m_response[m_responseRows][m_responseCharPosition++] = c;
			m_response[m_responseRows][m_responseCharPosition] = 0;

			if (m_responseCharPosition == MAX_DATA_GSM_SERIAL_BUF_LEN)
			{
				m_responseRows++;
				m_responseCharPosition = 0;
				m_response[m_responseRows][m_responseCharPosition] = 0;
			}

			if (m_responseRows == MAX_DATA_GSM_SERIAL_RESPONSE_ROWS)
			{
				break;
			}
		}

		if (waitForAll && m_response[m_responseRows][0] != 0 && strcmp(m_response[m_responseRows - 1], "OK") != 0)
			responseBytes = 0;

		unsigned long timeLen = millis() - now;
		
		if (timeLen > timeout)
		{
			log (shpllDebug, "ShpDataGSM::readResponse timeout");
			break;
		}
	}

	if (m_responseRows || m_responseCharPosition)
	{
		//printResponse();
		return true;
	}

	return false;
}

void ShpDataGSM::printResponse()
{
	Serial.println("module response:");
	for (int i = 0; i <= m_responseRows; i++)
	{
		//Serial.print(i);
		//Serial.print(": ");
		//Serial.println(m_response[i]);

		log (shpllDebug, "%d: %s", i, m_response[i]);
	}
	Serial.println("--- end ---");
}

void ShpDataGSM::initGSMModule()
{
	log (shpllDebug, "init...");
	while (!sendCmd("AT", "OK"))
	{
		log (shpllError, "ShpDataGSM::initGSMModule: module not working");
		printResponse();
		return;
	}
	//printResponse();

	delay(200);
	if (sendCmd("ATI", "OK"))
	{
		if (m_response[1][0] != 0)
			log (shpllDebug, m_response[1]);
	}
	
	delay(200);
	log(shpllDebug, "check network...");
	while (!sendCmd("AT+CREG?", "+CREG: 0,1"))
	{
		log (shpllStatus, "wait for gsm signal");
		//printResponse();
		return;
	}
	//printResponse();

	delay(200);
	log(shpllDebug, "check operator...");
	while (!sendCmd("AT+COPS?", "OK"))
	{
		log(shpllStatus, "wait for gsm operator");
		return;
	}
	//printResponse();

	// -- CLIP
	if (!sendCmd("AT+CLIP=1", "OK"))
	{

	}

	// -- set text SMS format (disable PDU)
	if (!sendCmd("AT+CMGF=1", "OK"))
	{
	}

	// -- disable on-fly SMS signalization
	if (!sendCmd("AT+CNMI=0,0", "OK"))
	{
	}

	// -- call progress monitoring
	if (!sendCmd("ATX4", "OK"))
	{
		Serial.println("ATX4 FAIL");
		printResponse();
	}
	if (!sendCmd("ATV1", "OK"))
	{
		Serial.println("ATV1 FAIL");
		printResponse();
	}
	if (!sendCmd("AT+MORING=1", "OK"))
	{
		Serial.println("AT+MORING=1 FAIL");
		printResponse();
	}

	m_moduleInitialized = true;
	log (shpllDebug, "ready");
}

bool ShpDataGSM::sendSMS (const char *number, const char *text)
{
	String cmd = "AT+CMGS=\"";
	cmd.concat(number);
	cmd.concat("\"");
	m_hwSerial->println(cmd);
	delay(100);

	m_hwSerial->println(text);
	m_hwSerial->write((char)26); // CTRL+Z
	delay(100);

	sendCmd("AT+CMGD=1", "OK");
	sendCmd("AT+CMGD=1,4", "OK");

	return true;	
}

bool ShpDataGSM::call (const char *number)
{
	String cmd = "ATD ";
	cmd.concat(number);
	cmd.concat(";");

	Serial.println ("CALL: " + cmd);
	m_hwSerial->println(cmd);
	readResponse();

	if (strcmp(m_response[1], "OK") != 0 && strcmp(m_response[1], "MO RING") != 0)
	{
		Serial.println("FAILED");
		printResponse();

		return false;
	}

	Serial.println("CALL OK");
	printResponse();

	m_outgoingCallStartedAt = millis();
	m_outgoingCallInProgress = true;
	
	return true;	
}

void ShpDataGSM::hangup()
{
	Serial.println ("HANG-UP1");
	if (!sendCmd("ATH", "NO CARRIER"))
	{
		Serial.println("hangup failed....");
		printResponse();
	}

	m_incomingCallPhoneNumber[0] = 0;
	m_incomingCallRingInProgress = 0;
}

bool ShpDataGSM::checkIncomingSMS()
{
	/*
	0: AT+CMGR=1
	1: +CMGR: "REC UNREAD","+420774077436","","20/06/11,14:01:44+08"
	2: Test 1
	3: 
	4: OK
	*/

	/*
	0: AT+CMGD=1
	1: OK
	2: 
	*/

	if (!sendCmd("AT+CMGR=1", "OK"))
	{
		log(shpllError, "check SMS failed");
		printResponse();
		//return false;
	}
	delay(100);

	if (m_responseRows < 1)
		return false;

	if (strcmp(m_response[0], "OK") == 0 || (m_responseRows > 1 && strcmp(m_response[1], "OK") == 0))
		return false;

	String smsText;

	for (int i = 0; i <= m_responseRows; i++)
	{
		if (strncmp(m_response[i], "+CMGR: ", 7))
			continue;

		char phoneNumber[MAX_PHONE_NUMBER_LEN] = "";
		if (searchPhoneNumber(m_response[i] + 6, phoneNumber))
		{
			Serial.printf("SMS FROM `%s`\n", phoneNumber);
			for (int t = i + 1; t <= m_responseRows - 2; t++)
			{
				if (m_response[t][0] == 0 && strcmp(m_response[t + 1], "OK") == 0 && m_response[t+2][0] == 0)
					break;

				if (smsText.length())
					smsText.concat("\\n");

				smsText.concat(m_response[t]);
			}

			String payload = "{\"action\": \"sms\", \"number\": \"";
			payload.concat(phoneNumber);
			payload.concat ("\", \"msg\": \"");
			payload.concat(smsText);
			payload.concat("\"}");

			app->publish(payload.c_str(), m_valueTopic.c_str());

			sendCmd("AT+CMGD=1", "OK");
			sendCmd("AT+CMGD=1,4", "OK");

			return true;
		}
	}

	return false;
}

void ShpDataGSM::checkModule()
{
	// -- UART 
	while (!sendCmd("AT", "OK"))
	{
		log (shpllError, "ShpDataGSM::checkModule: module not working");
		printResponse();
		//m_moduleInitialized = false;
		//m_nextCheck = millis() + 10000; // next try after 10 seconds
		return;
	}

	// -- signal quality
	if (!sendCmd("AT+CSQ", "OK"))
	{
		log (shpllDebug, "gsm signal level failed");
		return;
	}
	if (m_response[1][0] != '+' || m_response[1][4] != ':')
	{
		
		return;
	}

	char *signalQaulityStr = m_response[1] + 5;
	char *comma = strchr(signalQaulityStr, ',');
	if (comma)
		comma[0] = 0;

	int signalQuality = atoi(signalQaulityStr);

	
	if (signalQuality < 10)
	{
		log(shpllError, "bad GSM signal (%d)", signalQuality);
		return;
	}

	if (signalQuality < 15)
	{
		log(shpllError, "poor GSM signal (%d)", signalQuality);
		return;
	}
	
	if (signalQuality < 20)
	{
		log(shpllInfo, "GSM signal is good (%d)", signalQuality);
		return;
	}

	log(shpllInfo, "GSM signal is excellent (%d)", signalQuality);
}

bool ShpDataGSM::checkIncomingCall()
{
	/*
	0: AT+CNMI=0,0
	1: RING
	2: 
	3: +CLIP: "+420774077436",145,"",0,"",0
	4: 
	*/

	/*
	0: AT+CNMI=0,0
	1: NO CARRIER
	2: 
	*/

	if (!m_hwSerial->available())
		return false;

	readResponse();

	if (m_responseRows < 1)
		return false;

	if (strcmp(m_response[0], "NO CARRIER") == 0 || (m_responseRows > 1 &&strcmp(m_response[1], "NO CARRIER") == 0))
	{ // hangup
		m_incomingCallPhoneNumber[0] = 0;
		m_incomingCallRingInProgress = 0;

		//Serial.println ("hangup...");
		return false;
	}

	if (strcmp(m_response[0], "RING") != 0 && strcmp(m_response[1], "RING") != 0)
		return false;

	if (m_incomingCallRingInProgress > m_incomingCallMaxRings)
	{
		hangup();
		return true;
	}

	for (int i = 0; i <= m_responseRows; i++)
	{
		if (strncmp(m_response[i], "+CLIP: ", 7))
			continue;

		char phoneNumber[MAX_PHONE_NUMBER_LEN] = "";
		if (searchPhoneNumber(m_response[i] + 6, phoneNumber))
		{
			m_incomingCallRingInProgress++;

			if (strcmp (m_incomingCallPhoneNumber, phoneNumber) == 0)
			{ // same number
				//Serial.println ("ring...");
				return false;
			}
			//Serial.printf("INCOMING CALL FROM `%s`\n", phoneNumber);

			strcpy(m_incomingCallPhoneNumber, phoneNumber);

			String payload = "{\"action\": \"call\", \"number\": \"";
			payload.concat(phoneNumber);
			payload.concat("\"}");

			app->publish(payload.c_str(), m_valueTopic.c_str());

			return true;
		}
	}

	m_incomingCallRingInProgress = 0;
	m_incomingCallPhoneNumber[0] = 0;

	return false;
}

bool ShpDataGSM::checkOutgoingCall()
{
	if (!m_outgoingCallInProgress)
		return false;

	unsigned long callLen = (millis() - m_outgoingCallStartedAt) / 1000;
	if (callLen > m_outgoingCallMaxSecs)
	{
		//Serial.println("OC HANGUP1");
		hangup();
		m_outgoingCallInProgress = false;
		m_outgoingCallStartedAt = 0;
	}

	if (!m_hwSerial->available())
	{
		return true;
	}

	readResponse();
	//printResponse();

	if (
		strcmp(m_response[0], "BUSY") == 0 || (m_responseRows > 1 &&strcmp(m_response[1], "BUSY") == 0) ||
		strcmp(m_response[0], "NO CARRIER") == 0 || (m_responseRows > 1 &&strcmp(m_response[1], "NO CARRIER") == 0) ||
		strcmp(m_response[0], "NO ANSWER") == 0 || (m_responseRows > 1 &&strcmp(m_response[1], "NO ANSWER") == 0)
	)
	{ // hang up
		//Serial.println("OC HANGUP2");
		m_outgoingCallInProgress = false;
		m_outgoingCallStartedAt = 0;
	}

	return true;	
}

bool ShpDataGSM::searchPhoneNumber(const char *srcText, char *result)
{
	char *begin = strchr(srcText, '+');
	if (begin)
	{
		char *end = strchr(begin + 1, '"');
		if (end)
		{
			int len = end - begin;
			if (len > 1 && len < MAX_PHONE_NUMBER_LEN)
			{
				strncpy(result, begin, len);
				result[len] = 0;
				return true;
			}
		}
	}

	return false;
}

void ShpDataGSM::loop()
{
	ShpIOPort::loop();

	if (!m_hwSerial)
		return;

	unsigned long now = millis();
	if (now < m_nextCheck)
		return;

	if (!m_moduleInitialized)
	{
		initGSMModule();

		if (!m_moduleInitialized)
			m_nextCheck = now + 10000; // failed; next try after 10 seconds

		return;
	}

	m_nextCheck = now + m_checkInterval;	

	if (checkOutgoingCall())
		return;

	if (checkIncomingCall())
		return;

	if (m_incomingCallRingInProgress)
		return;

	if (checkIncomingSMS())
		return;

	if (m_nextModuleCheck < now)
	{
		checkModule();
		m_nextModuleCheck = now + m_checkModuleInterval;
	}

	for (int i = 0; i < DATA_GSM_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsDoIt)
			continue;
		
		if (m_queue[i].startAfter > now)
			continue;

		m_queue[i].qState = qsRunning;
		runQueueItem(i);

		break;
	}

	m_nextCheck = millis() + m_checkInterval;	
}

