extern SHP_APP_CLASS *app;


ShpGpioExpanderRS485::ShpGpioExpanderRS485() :
																						m_expType(SHP_GPIO_RS485_MODBUS_REL8_NONAME),
																						m_bus(NULL),
																						m_address(-1)
{
}

void ShpGpioExpanderRS485::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
	 	{
      "type": "gpio-expander/rs485",
      "portId": "relays",
      "busPortId": "rs485",
      "address": "01",
      "expType": 1
}
	-----------------------------*/

	m_address = 0x01;

	ShpIOPort::init(portCfg);

	// -- busPortid
	m_busPortId = NULL;
	if (portCfg["busPortId"] != nullptr)
		m_busPortId = portCfg["busPortId"].as<const char*>();

	if (!m_busPortId)
		return;

	// -- address
	if (portCfg["address"] != nullptr)
	{
		char *err;
		m_address = strtol(portCfg["address"], &err, 16);
		if (*err)
		{
			log (shpllError, "Invalid RS485 MODBUS address format");
			return;
		}
	}

	if (m_address < 1 || m_address > 250)
	{
		log (shpllError, "Invalid MODBUS address number");
		return;
	}

	// -- expType
	if (portCfg["expType"] != nullptr)
		m_expType = portCfg["expType"];

	if (m_expType < 0 || m_expType > SHP_GPIO_RS485_MAX_VALUE)
	{
		return;
	}

	m_valid = true;

	//Serial.printf("RS485 EXPANDER START: busPortId: `%s`, address=`%02x`, valid=`%d`\n", m_busPortId, m_address, m_valid);
}

void ShpGpioExpanderRS485::init2()
{
	if (!m_valid || !m_busPortId)
		return;

	m_bus = (ShpBusRS485*)app->ioPort(m_busPortId);

	if (m_bus)
	{
		delay(10);
	}
	else
	{
		log (shpllError, "RS485 bus not found");
	}
}

void ShpGpioExpanderRS485::setPinState(uint8_t pin, uint8_t value)
{
  if (m_expType == SHP_GPIO_RS485_MODBUS_REL8_NONAME)
  { // https://www.laskakit.cz/user/related_files/8ch_rele_rs485_protocols.pdf
	  byte cmd[] = {/*0x01*/ m_address, 0x06, 0x00, pin, 0x00, 0x00, 0x00, 0x00};
    if (value)
      cmd[4] = 0x01;
    else
      cmd[4] = 0x02;

    uint16_t crc = modbusCalcCRC(cmd, 6);
    cmd[6] = crc >> 8;
    cmd[7] = crc & 0x00ff;

    m_bus->addQueueItemWrite(8, cmd, 10);
  }
}

uint16_t ShpGpioExpanderRS485::modbusCalcCRC(uint8_t *buffer, uint8_t bufferLen)
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

void ShpGpioExpanderRS485::loop()
{
	ShpIOPort::loop();
}
