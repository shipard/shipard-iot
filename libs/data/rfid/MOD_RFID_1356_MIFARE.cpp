extern SHP_APP_CLASS *app;
extern int g_cntUarts;


ShpMODRfid1356Mifare::ShpMODRfid1356Mifare() :
																							m_speed(38400),
																							m_mode(SERIAL_8N1),
																							m_rxPin(-1),
																							m_txPin(-1),
																							m_revertCode(true),
																							m_hwSerial(NULL)
{
}

void ShpMODRfid1356Mifare::init(JsonVariant portCfg)
{
	/* MOD_IO_1356_MIFARE config format:
	 * --------------------------
	 {
			"type": "dataRFIDMOD1356MIFARE",
			"portId": "rfid",
			"pinRX": 16,
			"pinTX": 17
		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_buffer[0] = 0;
	m_sbCnt = 0;
	m_bufNeedSend = 0;
	m_lastCardId[0] = 0;
	m_lastTagIdReadMillis = 0;
	m_sameTagIdTimeout = 2000;

	// -- rx / tx pin
	if (portCfg["pinRX"] != nullptr)
		m_rxPin = portCfg["pinRX"];
	if (portCfg["pinTX"] != nullptr)
		m_txPin = portCfg["pinTX"];

	if (m_txPin < 0 || m_rxPin < 0)
	{
		log(shpllError, "wrong pin config: pinRX=`%d`, pinTX=`%d`", m_rxPin, m_txPin);
		return;
	}
	// -- portId buzzer
	if (portCfg.containsKey("portIdBuzzer"))
	{
		m_portIdBuzzer.concat((const char*)portCfg["portIdBuzzer"]);
	}

	log(shpllDebug, "configured: pinRX=`%d`, pinTX=`%d`, portBuzzer=`%s`", m_rxPin, m_txPin, m_portIdBuzzer.c_str());

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(m_speed, SERIAL_8N1, m_rxPin, m_txPin);
}

void ShpMODRfid1356Mifare::loop()
{
	if (!m_hwSerial)
		return;

	while (m_hwSerial->available())
	{
		char c = (char)m_hwSerial->read();

		if (c == 10 || c == 13)
		{
			m_bufNeedSend = 1;
			break;
		}

		m_buffer[m_sbCnt] = c;
		m_buffer[m_sbCnt + 1] = 0;

		m_sbCnt++;
		if (m_sbCnt == MAX_MOD_RFID_1356_MIFARE_BUF_LEN)
		{
			m_bufNeedSend = 1;
			break;
		}
	}

	if (m_bufNeedSend)
	{
		if (m_sbCnt)
		{
			//Serial.print("RFID: ");
			//Serial.println(m_buffer);
			sendData(m_buffer);
		}

		m_bufNeedSend = 0;
		m_sbCnt = 0;
		m_buffer[0] = 0;
	}

	ShpIOPort::loop();
}

void ShpMODRfid1356Mifare::sendData(const char *data)
{
	if (data[0] != '-')
	{

		return;
	}

	String code = "";
	if (m_revertCode)
	{
		size_t dataLen = strlen(data) - 1;
		while(dataLen > 0)
		{
			code += data[dataLen - 1];
			code += data[dataLen];
			dataLen -= 2;
		}
	}
	else
	{
		code += data + 1;
	}

	unsigned long now = millis();
	if (now - m_lastTagIdReadMillis < m_sameTagIdTimeout && strcmp(code.c_str(), m_lastCardId) == 0)
	{
		m_lastTagIdReadMillis = now;
		return;
	}

	strcpy(m_lastCardId, code.c_str());
	m_lastTagIdReadMillis = now;

	if (m_portIdBuzzer != "")
	{
		ShpIOPort *ioPortBuzzer = app->ioPort(m_portIdBuzzer.c_str());
		if (ioPortBuzzer)
		{
			ioPortBuzzer->onMessage((byte*)"P50", 3, NULL);
		}
	}

	app->publish(code.c_str(), m_valueTopic.c_str());
}
