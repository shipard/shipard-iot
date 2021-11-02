extern Application *app;


int g_cntUarts = 1;


ShpDataSerial::ShpDataSerial() : 
																m_speed(-1),
																m_mode(0),
																m_rxPin(-1), 
																m_txPin(-1),
																m_telnetPort(0),
																m_hwSerial(NULL),
																m_telnet(NULL)
{
}

void ShpDataSerial::init(JsonVariant portCfg)
{
	/* serial port config format:
	 * --------------------------
	 {
			"type": "dataSerial",
			"portId": "uio-5-1",
			"speed": "5",
			"pinRX": 16,
			"pinTX": 17,
			"mode": "1",
			"useTelnet": 1,
			"telnetPort": "5001"

		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_buffer[0] = 0;
	m_sbCnt = 0;
	m_bufNeedSend = 0;

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
		return;

	Serial.printf("SERIAL START: rx: %d, tx: %d\n", m_rxPin, m_txPin);

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(m_speed, m_mode, m_rxPin, m_txPin);

	if (portCfg.containsKey("useTelnet") && portCfg["useTelnet"] == 1)
	{
		m_telnetPort = (int)(portCfg["telnetPort"]);
		if (!m_telnetPort)
			m_telnetPort = 5000 + g_cntUarts - 1;
		m_telnet = new ShpTelnet(m_telnetPort);
	}	
}

void ShpDataSerial::loop()
{
	if (!m_hwSerial)
		return;

	if (m_telnet)
	{
		m_telnet->loop(m_hwSerial, m_hwSerial);
		ShpIOPort::loop();
		return;
	}

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
		if (m_sbCnt == MAX_DATA_SERIAL_BUF_LEN)
		{
			m_bufNeedSend = 1;
			break;
		}
	}

	if (m_bufNeedSend)
	{
		if (m_sbCnt)
		{
			app->publish(m_buffer, m_valueTopic.c_str());
		}
		
		m_bufNeedSend = 0;
		m_sbCnt = 0;
		m_buffer[0] = 0;
	}

	ShpIOPort::loop();
}

