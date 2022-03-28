extern SHP_APP_CLASS *app;



void stateChanged(bool plugged, ShpDataWiegand *ioPort) 
{
	Serial.print("[wiegand] - ");
  Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

void data2HexStr(uint8_t* data, size_t len, char str[]) 
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

void receivedData(uint8_t* data, uint8_t bits, ShpDataWiegand *ioPort) 
{
    uint8_t bytes = (bits+7)/8;
		data2HexStr(data, bytes, ioPort->m_buffer);
}

void receivedDataError(Wiegand::DataError error, uint8_t* rawData, uint8_t rawBits, ShpDataWiegand *ioPort) 
{
    Serial.print("[wiegand] - ERROR: ");
    Serial.print(Wiegand::DataErrorStr(error));
    Serial.print(" - Raw data: ");
    Serial.print(rawBits);
    Serial.print("bits / ");

    // Print value in HEX
    uint8_t bytes = (rawBits+7)/8;
    for (int i=0; i<bytes; i++) 
		{
        Serial.print(rawData[i] >> 4, 16);
        Serial.print(rawData[i] & 0xF, 16);
    }
    Serial.println();
}




ShpDataWiegand::ShpDataWiegand() : 
																	m_d0Pin(-1), 
																	m_d1Pin(-1),
																	m_wiegand(NULL)
{
}

void ShpDataWiegand::init(JsonVariant portCfg)
{
	/* wiegnand port config format:
	 * --------------------------
		{
			"type": "dataWiegand",
			"portId": "uio-6-4",
			"pinD0": 15,
			"pinD1": 2
		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_buffer[0] = 0;

	// -- d0 / d1 pin
	if (portCfg["pinD0"] != nullptr)
		m_d0Pin = portCfg["pinD0"];
	if (portCfg["pinD1"] != nullptr)
		m_d1Pin = portCfg["pinD1"];

	if (m_d0Pin < 0 || m_d1Pin < 0)
		return;

	// -- init
	pinMode(m_d0Pin, INPUT);
  pinMode(m_d1Pin, INPUT);

	m_wiegand = new Wiegand();

	m_wiegand->onStateChange(stateChanged, this);
	m_wiegand->onReceive(receivedData, this);
  m_wiegand->onReceiveError(receivedDataError, this);

	m_wiegand->begin(Wiegand::LENGTH_ANY, true);

	attachInterrupt(m_d0Pin, std::bind(&ShpDataWiegand::pinStateChanged, this, m_d0Pin), CHANGE);
	attachInterrupt(m_d1Pin, std::bind(&ShpDataWiegand::pinStateChanged, this, m_d1Pin), CHANGE);

	pinStateChanged(0);
}

void ShpDataWiegand::pinStateChanged(int pin) 
{
  m_wiegand->setPin0State(digitalRead(m_d0Pin));
  m_wiegand->setPin1State(digitalRead(m_d1Pin));
}


void ShpDataWiegand::loop()
{
	ShpIOPort::loop();

	if (!m_wiegand)
		return;

	noInterrupts();
  m_wiegand->flush();
  interrupts();

	if (m_buffer[0] != 0)
	{
		app->publish(m_buffer, m_valueTopic.c_str());
		m_buffer[0] = 0;
	}
}

