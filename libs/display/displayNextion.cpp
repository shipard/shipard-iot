extern Application *app;
extern int g_cntUarts;

void dataHexStr(uint8_t* data, size_t len, char str[]) 
{
	const char hex_str[]= "0123456789abcdef";

	size_t validChars = 0;

  for (uint8_t i = 0; i < len; i++)
	{
		uint8_t b = data[i];

		if (b == 0 && validChars == 0)
			continue;

   	str[validChars++] = hex_str[(b >> 4) & 0x0F];
   	str[validChars++] = hex_str[(b     ) & 0x0F];
  }

	str[validChars] = 0;
}

ShpDisplayNextion::ShpDisplayNextion() : 
																m_speed(-1),
																m_mode(0),
																m_rxPin(-1),
																m_txPin(-1),
																m_currentDimLevel(0),
																m_defaultDimLevel(100),
																m_hwSerial(NULL)
{
}

void ShpDisplayNextion::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 {
			"type": "displayNextion",
			"portId": "uio-5-1",
			"pinRX": 16,
			"pinTX": 17,
		}
	-----------------------------*/

	ShpDisplay::init(portCfg);

	// -- init members
	m_buffer[0] = 0;
	m_sbCnt = 0;
	m_bufNeedSend = 0;

	// -- port speed
	m_speed = 9600;

	// -- port mode (bits, parity, stop bits)
	m_mode = SERIAL_8N1;

	// -- rx / tx pin
	if (portCfg["pinRX"] != nullptr)
		m_rxPin = portCfg["pinRX"];
	if (portCfg["pinTX"] != nullptr)
		m_txPin = portCfg["pinTX"];

	if (m_txPin < 0 || m_rxPin < 0)
		return;

	//Serial.printf("DISPLAY NEXTION START: rx: %d, tx: %d\n", m_rxPin, m_txPin);

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(m_speed, m_mode, m_rxPin, m_txPin);

	initDisplay();
}

void ShpDisplayNextion::doCommand(const char *cmd)
{
	Serial.println("---DO COMMAND---");

	int len = m_hwSerial->print(cmd);
	if (len <= 0)
		Serial.println("---WRITE FAILED---");

	m_hwSerial->write(0xFF);
	m_hwSerial->write(0xFF);
	m_hwSerial->write(0xFF);
}

void ShpDisplayNextion::doCommand_Set(const char *itemId, const char *text)
{
	String cmd = itemId;
Serial.printf ("SET `%s` = `%s`\n", itemId, text);
	if (strcmp(itemId, "page") == 0)
	{
		cmd += " ";
		addEscapedText(cmd, text);
	}
	else if (strcmp(itemId, "dim") == 0)
	{
		if (strcmp(text, "_") == 0)
		{
			setDimLevel(0);
			return;
		}
		else if (strcmp(text, "!") == 0)
		{
			setDimLevel(m_defaultDimLevel);
			return;
		}

		setDimLevel(atoi(text));
		//cmd += "=";
		//addEscapedText(cmd, text);
	}
	else
	{
		cmd += "=\"";
		addEscapedText(cmd, text);
		cmd += "\"";
	}

	//Serial.println(cmd);
	doCommand (cmd.c_str());
}

void ShpDisplayNextion::readCommandResult()
{
	m_sbCnt = 0;
	m_buffer[0] = 0;

	while (m_hwSerial->available())
	{
		uint8_t c = (char)m_hwSerial->read();

		if (c == 0xFF && m_sbCnt >= 1 && m_buffer[m_sbCnt] == 0xFF && m_buffer[m_sbCnt - 1] == 0xFF)
		{
			m_bufNeedSend = 1;
			break;
		}

		m_buffer[m_sbCnt] = c;
		m_buffer[m_sbCnt + 1] = 0;

		m_sbCnt++;
		if (m_sbCnt == MAX_DISPLAY_NEXTION_BUF_LEN)
		{
			m_bufNeedSend = 1;
			break;
		}
	}

	char hex [500];
	dataHexStr(m_buffer, m_sbCnt, hex);
	//Serial.println (hex);

	/*
	for (int ii = 0; ii < m_sbCnt - 3; ii++)
	{
		char xx = (char)m_buffer[ii];
		if (xx >= 32)
			Serial.print(xx);
	}
	*/

	//if (m_sbCnt)
	//	Serial.println ("cr");
}

void ShpDisplayNextion::waitForCommandResult()
{
	m_buffer[0] = 0;

	readCommandResult();

	while(m_buffer[0] == 0)
	{
		delay(50);
		readCommandResult();
	}
}

void ShpDisplayNextion::waitForConfirm()
{
	while (!m_hwSerial->available())
	{
		delay(10);
	}

	while (m_hwSerial->available())
	{	
		uint8_t c = (char)m_hwSerial->read();
		if (c == 0x05)
			Serial.print("!");
	}
}

void ShpDisplayNextion::updateFirmware(const char *params)
{
	Serial.println("DISPLAY FIRMWARE UPDATE");

	String payload = params;

	int fwLenght = 0;
	String fwUrl = "";

	if (payload.length() == 0)
	{
		return;
	}

	
	int spacePos = payload.indexOf(' ');
	if (spacePos == -1)
	{
		Serial.printf("Invalid payload format\n");
		return;
	}

	String fwLenStr = payload.substring(0, spacePos);
	fwLenght = atoi(fwLenStr.c_str());
	fwUrl = payload.substring(spacePos + 1);

	if (fwLenght == 0)
	{
		Serial.printf("Invalid firmware lenght\n");
		return;
	}
	if (fwUrl.length() == 0)
	{
		Serial.printf("Blank firmware url\n");
		return;
	}
	

	if (fwLenght == 0 || fwUrl.length() == 0)
		return;

	Serial.printf("LEN: `%d`, URL: `%s`\n", fwLenght, fwUrl.c_str());

	ShpHttpRequest httpRequest;
	if (!httpRequest.begin(fwUrl.c_str()))
	{
		Serial.println("DOWNLOAD FAILED");
		return;
	}

	size_t totalBytes = 0;
	size_t readedBytes = 0;
	//uint8_t buffer[4097];

	WiFiClient stream = httpRequest.m_httpClient.getStream();

	String uploadCmd = "whmi-wri ";
	uploadCmd.concat (fwLenght);
	uploadCmd.concat (",");
	uploadCmd.concat (9600);
	uploadCmd.concat (",res0");

	Serial.println(uploadCmd);

	doCommand(uploadCmd.c_str());
	waitForConfirm();

	while (stream.available())
	{
		int c = stream.read();
		if (c < 0)
			break;
		//buffer[readedBytes]	= c;

		m_hwSerial->write(c);

		readedBytes++;
		if (readedBytes == 4096)
		{
			Serial.print(".");
			readedBytes = 0;

			waitForConfirm();
		}
		totalBytes += 1;
	}

	if (readedBytes)
	{
		waitForConfirm();
	}

	Serial.println();
	Serial.println(totalBytes);

	httpRequest.end();
}

void ShpDisplayNextion::addEscapedText(String &dst, const char *text)
{
	size_t c = 0;
	while(text[c])
	{
		if (text[c] == '"')
			dst.concat('\\');
		else if (text[c] == '\\')
			dst.concat('\\');
		else if (text[c] == '\n')
		{
			dst.concat("\\r");
		}
		dst.concat((char)text[c]);

		c++;
	}
}

void ShpDisplayNextion::setDimLevel(uint8_t level)
{
	if (level > 100)
		level = 100;

	char cmd[16];

	if (level == 0)
	{
		for (int l = m_currentDimLevel; l > 0; l -= 10)
		{
			sprintf(cmd, "dim=%d", l);
			doCommand(cmd);
			delay(40);
		}
	}
	else
	{
		for (int l = m_currentDimLevel; l < m_defaultDimLevel; l += 10)
		{
			sprintf(cmd, "dim=%d", l);
			doCommand(cmd);
			delay(40);
		}
	}
	sprintf(cmd, "dim=%d", level);
	//Serial.println(cmd);
	doCommand(cmd);
	m_currentDimLevel = level;
}

void ShpDisplayNextion::runQueueItem(int i)
{
	Serial.println("---RUN---");
	if (strcmp(m_queue[i].subCmd, "touch-calibration") == 0)
	{
		doCommand("touch_j");

	}
	else if (strcmp(m_queue[i].subCmd, "firmware") == 0)
	{
			updateFirmware(m_queue[i].payload.c_str());
	}
	else if (strcmp(m_queue[i].subCmd, "set") == 0)
	{
		if (m_queue[i].payload[0] == '{')
		{
			StaticJsonDocument<4096> data;
			DeserializationError error = deserializeJson(data, m_queue[i].payload);
			if (error) 
			{
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.c_str());
				return;
			}

			JsonObject setItems = data["set"];
			for (JsonPair oneItem: setItems)
			{
				doCommand_Set(oneItem.key().c_str(), oneItem.value().as<char*>());
			}
		}
		else
		{
			char *separator = strchr(m_queue[i].payload.begin(), ':');
			if (separator != NULL)
			{
				separator[0]	= 0;

				doCommand_Set(m_queue[i].payload.c_str(), separator + 1);
			}
		}
	}

	ShpDisplay::runQueueItem(i);
}

void ShpDisplayNextion::initDisplay()
{
	doCommand("connect");

	int retries = 0;
	while(m_buffer[0] == 0)
	{
		delay(50);
		readCommandResult();
		retries++;

		if (retries > 50)
		{
			log (shpllError, "display not found");
			return;
		}
	}

	// -- comok 1,30601-0,NX3224T024_011R,142,61488,DE69905787617823,4194304

	int paramNdx = 0;
	char *oneItem = strtok((char*)m_buffer, ",");
	while (oneItem != NULL)
	{
		if (paramNdx == 2)
		{
			Serial.println(oneItem);
			m_displayType = oneItem;
		}
		else if (paramNdx == 5)
		{
			Serial.println(oneItem);
			m_displaySN = oneItem;
		}
		oneItem = strtok (NULL, ",");
		paramNdx++;
	}

	if (m_displayType != "" && m_displaySN != "")
	{
		log(shpllDebug, "displayType: %s, sn: %s", m_displayType.c_str(), m_displaySN.c_str());
	}
	else
	{
		log(shpllError, "wrong displayType/sn detect: type: `%s`, sn: `%s`", m_displayType.c_str(), m_displaySN.c_str());
	}
	
	setDimLevel(0);
	doCommand("page 0");
	setDimLevel(m_defaultDimLevel);


	doCommand("page 1");
}

void ShpDisplayNextion::loop()
{
	if (!m_hwSerial)
		return;

	if (m_hwSerial->available())
		readCommandResult();
	
	ShpDisplay::loop();
}
