extern SHP_APP_CLASS *app;


int g_cntI2C = 1;


ShpBusI2C::ShpBusI2C() :
												m_pinSDA(-1),
												m_pinSCL(-1),
												m_wire(NULL)
{
}

void ShpBusI2C::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 {
			"type": "busI2C",
			"portId": "uio-5-1",
			"pinSDA": 16,
			"pinSCL": 17
		}
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- SDA / SCL pin
	if (portCfg["pinSDA"] != nullptr)
		m_pinSDA = portCfg["pinSDA"];
	if (portCfg["pinSCL"] != nullptr)
		m_pinSCL = portCfg["pinSCL"];

	if (m_pinSDA < 0 || m_pinSCL < 0)
		return;

	//Serial.printf("I2C START: sda: %d, scl: %d\n", m_pinSDA, m_pinSCL);

	m_wire = new TwoWire(g_cntI2C++);
	m_wire->begin(m_pinSDA, m_pinSCL, 100000L);

	//delay(250);
	scan();
}

void ShpBusI2C::scan()
{
	char addrHex[5];

	byte error;
  int cntDevices = 0;

  for (int address = 1; address < 127; address++ )
	{
    m_wire->beginTransmission (address);
    error = m_wire->endTransmission ();
    if (error == 0)
		{
			sprintf (addrHex, "0x%02x", address);
			if (cntDevices)
				m_devices.concat (" ");
			m_devices.concat (addrHex);
      cntDevices++;
    }
    else if (error == 4)
		{
			log(shpllError, "[I2C] Unknow error at address %02x", address);
    }
  }
  if (cntDevices == 0)
	{
		log(shpllError, "No I2C devices found");
  }
  else
	{
		log(shpllDebug, "I2C devices: %s", m_devices.c_str());

		Serial.printf("####### I2C devices: %s \n", m_devices.c_str());
	}
}

void ShpBusI2C::write16(int address, uint16_t data)
{
	int res = 0;

  m_wire->beginTransmission(address);
  res += m_wire->write(lowByte(data));
  res += m_wire->write(highByte(data));
  m_wire->endTransmission();

	log(shpllVerbose, "i2c write16: value=%d, bytes=%d", data, res);
}

void ShpBusI2C::write2B(int address, uint8_t data1, uint8_t data2)
{
	int res = 0;

  m_wire->beginTransmission(address);
  res += m_wire->write(data1);
  res += m_wire->write(data2);
  m_wire->endTransmission();

	log(shpllVerbose, "i2c write2B: data1=%d, data2=%d, bytes=%d", data1, data2, res);
}

void ShpBusI2C::loop()
{
	ShpIOPort::loop();
}
