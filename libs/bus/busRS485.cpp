extern SHP_APP_CLASS *app;
extern int g_cntUarts;
void data2HexStr(uint8_t* data, size_t len, char str[]);


ShpBusRS485::ShpBusRS485() :
                            m_speed(-1),
                            m_mode(0),
                            m_rxPin(-1),
                            m_txPin(-1),
                            m_controlPin(-1),
                            m_hwSerial(NULL),
														m_queueRequests(0)
{
	for (int i = 0; i < BUS_RS485_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].queueItemType = BUS_RS485_ITEM_TYPE_NONE;
		m_queue[i].bufferLen = 0;
	}
}

void ShpBusRS485::init(JsonVariant portCfg)
{
	/* serial port config format:
	 * --------------------------
	{
		"type": "bus/rs485",
		"portId": "rs485",
		"speed": 5,
		"mode": 0,
		"pinRX": 36,
		"pinTX": 4,
		"pinControl": 32
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

	// -- rx / tx pin / control
	if (portCfg.containsKey("pinRX"))
		m_rxPin = portCfg["pinRX"];
	if (portCfg.containsKey("pinTX"))
		m_txPin = portCfg["pinTX"];
	if (portCfg.containsKey("pinControl"))
		m_controlPin = portCfg["pinControl"];

	if (m_txPin < 0 || m_rxPin < 0 || m_speed < 1 || m_mode < 1)
		return;

	//Serial.printf("RS485 START: rx: %d, tx: %d, control: %d\n", m_rxPin, m_txPin, m_controlPin);

	m_valueTopic = MQTT_TOPIC_THIS_DEVICE "/";
	m_valueTopic.concat(app->m_deviceId);
	m_valueTopic.concat("/");
	m_valueTopic.concat(m_portId);

	pinMode(m_controlPin, OUTPUT);
	digitalWrite(m_controlPin, LOW);

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(m_speed, m_mode, m_rxPin, m_txPin);
}

uint16_t ShpBusRS485::modbusCalcCRC(uint8_t *buffer, uint8_t bufferLen)
{ // https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino/blob/master/ModbusRtu.h
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < bufferLen; i++)
    {
			temp = temp ^ buffer[i];
			for (unsigned char j = 1; j <= 8; j++)
			{
				flag = temp & 0x0001;
				temp >>=1;
				if (flag)
					temp ^= 0xA001;
			}
    }

    // -- reverse byte order
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;

		// the returned value is already swapped
    // crcLo byte is first & crcHi byte is last
    return temp;
}

void ShpBusRS485::addQueueItemWrite(int8_t bufferLen, const byte* buffer, unsigned long startAfter)
{
	for (int i = 0; i < BUS_RS485_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + startAfter;
		m_queue[i].queueItemType = BUS_RS485_ITEM_TYPE_WRITE_BUFFER;
		m_queue[i].bufferLen = bufferLen;
		for (int bp = 0; bp < bufferLen; bp++)
			m_queue[i].buffer[bp] = buffer[bp];

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpBusRS485::runQueueItem(int i)
{
	m_queueRequests--;

	if (m_queue[i].queueItemType == BUS_RS485_ITEM_TYPE_WRITE_BUFFER)
	{
		Serial.println("--RS485 write---");
		if (m_controlPin >= 0)
			digitalWrite(m_controlPin, HIGH);
		//delay(30);

		m_hwSerial->write(m_queue[i].buffer, m_queue[i].bufferLen);
		m_hwSerial->flush();

		//delay(10);
		if (m_controlPin >= 0)
			digitalWrite(m_controlPin, LOW);
		delay(50);
	}

	m_queue[i].qState = qsFree;
}


void ShpBusRS485::loop()
{
	if (!m_hwSerial)
		return;

	while (m_hwSerial->available())
	{
		char c = (char)m_hwSerial->read();
		//Serial.printf ("%02x ", c);

		/*
		if (c == 10 || c == 13)
		{
			m_bufNeedSend = 1;
			break;
		}
		*/

		m_buffer[m_sbCnt] = c;
		m_buffer[m_sbCnt + 1] = 0;
		m_sbCnt++;
		if (m_sbCnt == MAX_DATA_RS485_BUF_LEN)
		{
			m_bufNeedSend = 1;
			break;
		}
	}

	if (m_sbCnt)
		m_bufNeedSend = 1;

	if (m_bufNeedSend)
	{
		if (m_sbCnt)
		{
			static char hx[512];
			data2HexStr((uint8_t*)m_buffer, m_sbCnt, hx);
			Serial.println(hx);

			app->publish(hx, m_valueTopic.c_str());
		}

		m_bufNeedSend = 0;
		m_sbCnt = 0;
		m_buffer[0] = 0;
	}

	if (m_queueRequests != 0)
	{
		unsigned long now = millis();
		for (int i = 0; i < BUS_RS485_CONTROL_QUEUE_LEN; i++)
		{
			if (m_queue[i].qState != qsDoIt)
				continue;

			if (m_queue[i].startAfter > now)
				continue;

			m_queue[i].qState = qsRunning;
			runQueueItem(i);

			break;
		}
	}

	ShpIOPort::loop();
}
