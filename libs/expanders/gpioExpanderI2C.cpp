extern SHP_APP_CLASS *app;


ShpGpioExpanderI2C::ShpGpioExpanderI2C() : 
																						m_expType(SHP_GPIO_CHIP_PCF8575),
																						m_bus(NULL),
																						m_address(-1),
																						m_bits(0),
																						m_inputBits(0),
																						m_nextDigitalInputRead(0)
{
}

void ShpGpioExpanderI2C::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
			"type": "gpioExpanderI2C",
			"portId": "uio-5-1",
			"i2cBusPortId": "i2c_1",
			"address": "A0",
			"expType": 0,
			"dir": "0"
		}
	-----------------------------*/

	m_address = 0x20;

	ShpIOPort::init(portCfg);

	// -- busPortid
	m_busPortId = NULL;
	if (portCfg["i2cBusPortId"] != nullptr)
		m_busPortId = portCfg["i2cBusPortId"];

	if (!m_busPortId)
		return;

	// -- address
	if (portCfg["address"] != nullptr)
	{
		char *err;
		m_address = strtol(portCfg["address"], &err, 16);
		if (*err) 
		{
			log (shpllError, "Invalid I2C address format");
			return;	
		}		
		
	}
	if (m_address < 0 || m_address > 127)
	{
		log (shpllError, "Invalid I2C address number");
		return;
	}

	// -- expType
	if (portCfg["expType"] != nullptr)
		m_expType = portCfg["expType"];

	if (m_expType < 0 || m_expType > SHP_GPIO_CHIP_MAX_VALUE)	
	{
		return;
	}

	m_valid = true;	

	//Serial.printf("GPIO EXPANDER START: busPortId: `%s`, address=`%02x`, valid=`%d`\n", m_busPortId, m_address, m_valid);
}

void ShpGpioExpanderI2C::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);

	if (m_bus)
	{		
		delay(100);
		if (m_expType == SHP_GPIO_CHIP_PCF8575)
		{
			write(word(B11111111,B11111111));
			delay(100);	
			write(m_bits);
		}
		else if (m_expType == SHP_GPIO_CHIP_MCP23017)
		{
			write2B(0, 0);
			write2B(1, 0);
			delay(100);	
			write2B(0x12, lowByte(m_bits));
			write2B(0x13, highByte(m_bits));
		}
	}
	else
	{
		log (shpllError, "I2C bus not found");
	}
}

void ShpGpioExpanderI2C::setPinState(uint8_t pin, uint8_t value)
{
	if (value)
		m_bits |= bit(pin);
	else	
		m_bits &= ~bit(pin);

	if (m_expType == SHP_GPIO_CHIP_PCF8575)
	{
		write(m_bits);
	}
	else if (m_expType == SHP_GPIO_CHIP_MCP23017)
	{
		write2B(0x12, lowByte(m_bits));
		write2B(0x13, highByte(m_bits));
	}
	else if (m_expType == SHP_GPIO_OLIMEX_MODIO)
	{
		m_bus->write2B(m_address, 0x10, lowByte(m_bits));
	}
}

void ShpGpioExpanderI2C::write(uint16_t data)
{
	if (!m_valid || !m_bus)
		return;

  m_bus->write16(m_address, data);
}

void ShpGpioExpanderI2C::write2B(uint8_t data1, uint8_t data2)
{
	if (!m_valid || !m_bus)
		return;

  m_bus->write2B(m_address, data1, data2);
}

void ShpGpioExpanderI2C::readDigitalInput()
{
	uint8_t data = 0;

	if (m_expType == SHP_GPIO_OLIMEX_MODIO)
	{
		m_bus->wire()->beginTransmission(m_address);
		m_bus->wire()->write(0x20);
		m_bus->wire()->endTransmission();
		m_bus->wire()->requestFrom((int)m_address, 1);
		data = m_bus->wire()->read();

		for (uint8_t i = 4; i < 8; i++)
		{
			uint8_t newValue = (data & (bit(i - 4))) ? 1 : 0;
			uint8_t oldValue = (m_bits & (bit(i))) ? 1 : 0;

			if (newValue != oldValue || !m_nextDigitalInputRead)
			{
				if (newValue)
					m_bits |= bit(i);
				else	
					m_bits &= ~bit(i);
				
				if (m_inputIOPorts[i])
					m_inputIOPorts[i]->setValue(newValue);
			}
		}
	}
}

void ShpGpioExpanderI2C::loop()
{
	ShpIOPort::loop();

	if (m_expType == SHP_GPIO_OLIMEX_MODIO)
	{
		unsigned long now = millis();
		if (now < m_nextDigitalInputRead)
			return;

		readDigitalInput();

		m_nextDigitalInputRead = now + 250;
	}
}
