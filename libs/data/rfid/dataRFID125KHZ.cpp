extern Application *app;
extern int g_cntUarts;


ShpDataRFID125KHZ::ShpDataRFID125KHZ() : 
																		m_pinRX(-1), 
																		m_pinTX(-1),
																		m_sameTagIdTimeout(4000),
																		m_lastTagId(0),
																		m_lastTagIdReadMillis(0),
																		m_hwSerial(NULL)
{
}

void ShpDataRFID125KHZ::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 {
			"type": "dataRFID125kHz",
			"portId": "uio-5-1",
			"pinRX": 16,
			"pinTX": 17
		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_buffer[0] = 0;

	int speed = 9600;
	int mode = SERIAL_8N1;

	// -- rx / tx pin
	if (portCfg["pinRX"] != nullptr)
		m_pinRX = portCfg["pinRX"];
	if (portCfg["pinTX"] != nullptr)
		m_pinTX = portCfg["pinTX"];

	if (m_pinTX < 0 || m_pinRX < 0)
	{
		Serial.printf("RFID125KHZ CFG ERROR: rx: %d, tx: %d", m_pinRX, m_pinTX);
		return;
	}

	//Serial.printf("RFID125KHZ START: rx: %d, tx: %d", m_pinRX, m_pinTX);

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(speed, mode, m_pinRX, m_pinTX);
}

long ShpDataRFID125KHZ::extract_tag(uint8_t *buffer) 
{
	const int DATA_SIZE = 10; 				// 10byte data (2byte version + 8byte tag)
	const int DATA_VERSION_SIZE = 2; 	// 2byte version (actual meaning of these two bytes may vary)
	const int DATA_TAG_SIZE = 8; 			// 8byte tag
	const int CHECKSUM_SIZE = 2; 			// 2byte checksum

	uint8_t msg_head = buffer[0];
	uint8_t *msg_data = buffer + 1; // 10 byte => data contains 2byte version + 8byte tag
	uint8_t *msg_data_version = msg_data;
	uint8_t *msg_data_tag = msg_data + 2;
	uint8_t *msg_checksum = buffer + 11; // 2 byte
	uint8_t msg_tail = buffer[13];
    
  long tag = hexstr_to_value((char*)msg_data_tag, DATA_TAG_SIZE);
	long checksum = 0;
	for (int i = 0; i < DATA_SIZE; i+= 2) 
	{
		long val = hexstr_to_value((char*)msg_data + i, 2);
		checksum ^= val;
	}
	
	if (checksum == hexstr_to_value((char*)msg_checksum, 2))
		return tag;
	
  return 0; 
}

long ShpDataRFID125KHZ::hexstr_to_value(char *str, unsigned int length) 
{
  char copy[20];
  strncpy(copy, str, length);
  copy[length] = 0; 
  long value = strtol(copy, NULL, 16);
  return value;
}

void ShpDataRFID125KHZ::loop()
{
	if (!m_hwSerial)
		return;

	ShpIOPort::loop();
	
	int sbCnt = 0;
	bool doExtractTag = false;

	while (m_hwSerial->available()) 
	{
		int c = (char)m_hwSerial->read();
		if (c == -1)
		{ // read error
		  //Serial.println("READ ERROR");
			return;
		}

		if (c == 2) // tag begin
			sbCnt = 0;
		else if (c == 3) // tag readed
		{
			doExtractTag = true;
			break;
		}	

		m_buffer[sbCnt++] = c;

		if (sbCnt >= MAX_DATA_RFID_125KHZ_BUF_LEN)
		{
			//Serial.println("TAG ERROR");
			return;
		}
	}
	
	if (!doExtractTag)
		return;
	
	long tagId = extract_tag(m_buffer);
	if (!tagId)
		return;

	unsigned long now = millis();	
	if (now - m_lastTagIdReadMillis < m_sameTagIdTimeout && tagId == m_lastTagId)
	{
		m_lastTagIdReadMillis = now;
		return;
	}
	
	m_lastTagIdReadMillis = now;
	m_lastTagId = tagId;
	
	char buffer[16];
	sprintf(buffer, "%010ld", tagId);
	Serial.println(buffer);
	app->publish(buffer, m_valueTopic.c_str());	
}

