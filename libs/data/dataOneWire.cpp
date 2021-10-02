extern Application *app;


ShpDataOneWire::ShpDataOneWire() : 
																	m_oneWire(NULL), 
																	m_sensors(NULL), 
																	m_pin(-1), 
																	m_readInterval(60 * 1000),
																	m_lastRead(0),
																	m_countDevices(0),
																	m_sensorsStates(NULL)
{
}

void ShpDataOneWire::init(JsonVariant portCfg)
{
	/* 1-Wire port config format:
	 * --------------------------
	{
		"type": "dataOneWire",
		"portId": "uio-6-3",
		"pin": 5
  }
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- pin
	if (portCfg["pin"] != nullptr)
		m_pin = portCfg["pin"];

	if (m_pin < 0)
		return;

	m_oneWire = new OneWire(m_pin);
	scan();
	m_sensors = new DallasTemperature(m_oneWire);


	scanDevices();
}

void ShpDataOneWire::loop()
{
	unsigned long thisRead = millis();	
	
	if (thisRead - m_lastRead < m_readInterval)
		return;

	if (!m_countDevices)
		scanDevices();

	readValues();

	m_lastRead = thisRead;
}

void ShpDataOneWire::scan()
{
	uint8_t address[8];
  uint8_t count = 0;
	Serial.println ("1-Wire SCAN BUS....");

	m_oneWire->reset_search();

  if (m_oneWire->search(address))
  {
    Serial.println("[][8] = {");
    do {
      count++;
      Serial.println("  {");
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10) Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7) Serial.print(", ");
      }
      Serial.println("  },");
    } while (m_oneWire->search(address));

    Serial.println("};");
    Serial.print("// nr devices found: ");
    Serial.println(count);
  }
	else
	{
		Serial.println("...none...");
	}

	m_oneWire->reset_search();
}

void ShpDataOneWire::scanDevices()
{
	//m_oneWire->reset_search();

	m_sensors->begin();
	
	m_countDevices = m_sensors->getDeviceCount();

	log (shpllInfo, "%d sensors found", m_countDevices);

	if (m_sensorsStates)
	{
		delete m_sensorsStates;
		m_sensorsStates = NULL;
	}

	if (!m_countDevices)
		return;

	m_sensorsStates = new oneSensorState[m_countDevices];

	for(int i=0;i<m_countDevices; i++)
	{
		m_sensorsStates[i].deviceAddressStr[0] = 0;

		if(m_sensors->getAddress(m_sensorsStates[i].deviceAddress, i))
		{
			sensorAddressStr(m_sensorsStates[i].deviceAddress, m_sensorsStates[i].deviceAddressStr);
			strcpy(m_sensorsStates[i].topic, m_valueTopic.c_str());
			strcat(m_sensorsStates[i].topic, "temperature/");
			strcat(m_sensorsStates[i].topic, app->m_deviceId.c_str());
			strcat(m_sensorsStates[i].topic, "/");
			strcat(m_sensorsStates[i].topic, m_sensorsStates[i].deviceAddressStr);
		}
		else 
		{
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.println (" but could not detect address. Check power and cabling...");
		}
	}	
}

void ShpDataOneWire::readValues()
{
	m_sensors->requestTemperatures();
	for (int i = 0; i < m_countDevices; i++)
	{
		m_sensorsStates[i].newTemp = round(m_sensors->getTempC(m_sensorsStates[i].deviceAddress) * 10) / 10;
  }

	// StaticJsonDocument<512> dataRec;
	String payload;
	for (int i = 0; i < m_countDevices; i++)
	{
		if (1)
		{
			payload.clear();
			
			/* -- json way
			dataRec["id"] = m_sensorsStates[i].deviceAddressStr;
			dataRec["value"] = m_sensorsStates[i].newTemp;
			serializeJson(dataRec, payload);
			app->publish(payload.c_str(), m_valueTopic.c_str());
			*/

			payload.concat(m_sensorsStates[i].newTemp);
			app->publish(payload.c_str(), m_sensorsStates[i].topic);
			
			m_sensorsStates[i].pastTemp = m_sensorsStates[i].newTemp;
		}
	}
}

void ShpDataOneWire::sensorAddressStr(DeviceAddress deviceAddress, char address[]) 
{
	const char hex_str[]= "0123456789abcdef";

  for (uint8_t i = 0; i < 8; i++)
	{
		uint8_t b = deviceAddress[i];
   	address[i * 2 + 0] = hex_str[(b >> 4) & 0x0F];
   	address[i * 2 + 1] = hex_str[(b     ) & 0x0F];
  }

	address[16] = 0;
}
