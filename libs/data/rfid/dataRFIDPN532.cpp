extern SHP_APP_CLASS *app;
extern int g_cntUarts;
void data2HexStr(uint8_t* data, size_t len, char str[]);

void data2HexStrReverse(uint8_t data[], size_t len, char str[]) 
{
	const char hex_str[]= "0123456789abcdef";

	size_t validChars = 0;

	for (uint8_t i = 0; i < len; i++)
	{
		uint8_t b = data[len - i - 1];

		if (b == 0 && validChars == 0)
			continue;

   	str[validChars++] = hex_str[(b >> 4) & 0x0F];
   	str[validChars++] = hex_str[(b     ) & 0x0F];
  }

	str[validChars] = 0;
}


ShpDataRFIDPN532::ShpDataRFIDPN532() : 
																m_mode(PN532_MODE_I2C),
																m_pinRX(-1), 
																m_pinTX(-1),
																m_hwSerial(NULL),
																m_pn532hsu(NULL),
																m_busPortId(NULL),
																m_bus(NULL),
																m_pn532i2c(NULL)
{
}

void ShpDataRFIDPN532::init(JsonVariant portCfg)
{
	/* config format:
	 * --------------------------
 	{
		"type": "dataRFIDPN532",
		"portId": "rfid1",
		"valueTopic": "shp/sensors/unknown_thing_on_ioPort_6_14/rfid1",
		"pinHsuRX": 36,
		"pinHsuTX": 4,
		"portIdBuzzer": "buzzer"
  }
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_lastCardId[0] = 0;
	m_lastTagIdReadMillis = 0;
	m_sameTagIdTimeout = 2000;

	// -- mode I2C/HSU
	if (portCfg.containsKey("mode"))
		m_mode = portCfg["mode"];

	if (m_mode != PN532_MODE_I2C && m_mode != PN532_MODE_HSU)
	{
		log(shpllError, "Invalid cfg: mode");
		return;
	}

	// -- busPortid
	m_busPortId = NULL;
	if (m_mode == PN532_MODE_I2C)
	{
		if (portCfg["i2cBusPortId"] != nullptr)
			m_busPortId = portCfg["i2cBusPortId"];

		if (!m_busPortId)
		{
			log(shpllError, "Invalid cfg: i2cBusPortId");
			return;
		}
	}

	// -- HSU rx / tx pin
	if (m_mode == PN532_MODE_HSU)
	{
		if (portCfg["pinRX"] != nullptr)
			m_pinRX = portCfg["pinRX"];
		if (portCfg["pinTX"] != nullptr)
			m_pinTX = portCfg["pinTX"];

		if (m_pinRX < 0 || m_pinTX < 0)
		{
			log(shpllError, "Invalid cfg: HSU pins");
			return;
		}
	}

	// -- portId buzzer
	if (portCfg["portIdBuzzer"] != nullptr)
	{
		m_portIdBuzzer.concat((const char*)portCfg["portIdBuzzer"]);
		//Serial.printf("portIdBuzzer=`%s`\n", m_portIdBuzzer.c_str());
	}

	m_valid = true;
}

void ShpDataRFIDPN532::init2()
{
	if (!m_valid)
		return;

	if (m_mode == PN532_MODE_I2C)
	{
		if (!m_busPortId)
			return;

		m_bus = (ShpBusI2C*)app->ioPort(m_busPortId);
		if (!m_bus)
		{
			m_valid = false;
			log (shpllError, "I2C bus `%s` not found", m_busPortId);
			return;
		}
		m_pn532i2c = new PN532_I2C(*m_bus->wire());
		m_nfc = new PN532(*m_pn532i2c);
	}
	else
	{ // PN532_MODE_HSU
		m_hwSerial = new HardwareSerial(g_cntUarts++);
		m_hwSerial->begin(115200, SERIAL_8N1, m_pinRX, m_pinTX);
		sleep(1);

		m_pn532hsu = new PN532_HSU(*m_hwSerial);
		m_nfc = new PN532(*m_pn532hsu);
		m_nfc->begin();
	}
		
	usleep(100000);
	Serial.println("getFirmwareVersion1");
	uint32_t versiondata = m_nfc->getFirmwareVersion();
	usleep(120000);
	Serial.println("getFirmwareVersion2");
	versiondata = m_nfc->getFirmwareVersion();
	if (!versiondata) 
	{
		log(shpllWarning, "Didn't find PN53x board");
	}
	else
	{
		log (shpllDebug, "chip PN5%02x; fw %d.%d", (versiondata>>24) & 0xFF, (versiondata>>16) & 0xFF, (versiondata>>8) & 0xFF);
	}

	m_nfc->setPassiveActivationRetries(0x01);
	m_nfc->SAMConfig();
}

void ShpDataRFIDPN532::readTag()
{
	boolean success;
  uint8_t uid [] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  success = m_nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) 
	{
		char buffer[MAX_DATA_RFID_PN532_BUF_LEN];
		data2HexStrReverse(uid, uidLength, buffer);
		
		unsigned long now = millis();	
		if (now - m_lastTagIdReadMillis < m_sameTagIdTimeout && strcmp(buffer, m_lastCardId) == 0)
		{
			m_lastTagIdReadMillis = now;
			return;
		}

		strcpy(m_lastCardId, buffer);
		m_lastTagIdReadMillis = now;

		if (m_portIdBuzzer != "")
		{
			ShpIOPort *ioPortBuzzer = app->ioPort(m_portIdBuzzer.c_str());
			if (ioPortBuzzer)
			{
				ioPortBuzzer->onMessage("", "", (byte*)"P50", 3);
			}
		}

		app->publish(buffer, m_valueTopic.c_str());
		//Serial.println(buffer);
  }
}

void ShpDataRFIDPN532::loop()
{
	ShpIOPort::loop();
	if (!m_valid || m_paused)
		return;
	
	readTag();
}


#include <PN532.cpp>
#include <PN532_I2C.cpp>
#include <PN532_HSU.cpp>
