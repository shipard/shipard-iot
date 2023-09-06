extern SHP_APP_CLASS *app;
extern int g_cntUarts;




ShpSensorLD2410::ShpSensorLD2410() :
																m_pinRX(-1),
																m_pinTX(-1),
																m_nextMeasure(0),
																m_radar(NULL),
																m_hwSerial(NULL)
{
}

void ShpSensorLD2410::init(JsonVariant portCfg)
{

	ShpIOPort::init(portCfg);

	// -- init members


	// -- rx / tx pin
	if (portCfg.containsKey("pinRX"))
		m_pinRX = portCfg["pinRX"];
	if (portCfg.containsKey("pinTX"))
		m_pinTX = portCfg["pinTX"];

	if (m_pinTX < 0 || m_pinRX < 0)
		return;

	m_hwSerial = new HardwareSerial(g_cntUarts++);
	m_hwSerial->begin(256000, SERIAL_8N1, m_pinRX, m_pinTX);

	m_radar = new ld2410();
	if (m_radar->begin(*m_hwSerial))
	{
    Serial.println(F("OK"));
    Serial.print(F("LD2410 firmware version: "));
    Serial.print(m_radar->firmware_major_version);
    Serial.print('.');
    Serial.print(m_radar->firmware_minor_version);
    Serial.print('.');
    Serial.println(m_radar->firmware_bugfix_version, HEX);

		char idStill[] = "gt0s";
		char idMove[] = "gt0m";
		for (uint8_t gg = 0; gg <= 8; gg++)
		{
			idStill[2] = 48 + gg;
			idMove[2] = 48 + gg;

			int valueStill = 0;
			int valueMove = 0;

			if (portCfg.containsKey(idStill))
				valueStill = portCfg[idStill];
			if (portCfg.containsKey(idMove))
				valueMove = portCfg[idMove];

			Serial.printf("G: `%s`/`%s` [`%d`], S: %d, M: %d \n", idStill, idMove, gg, valueMove, valueStill);

			m_radar->setGateSensitivityThreshold(gg, valueMove, valueStill);

			//m_radar->requestFactoryReset();
			//m_radar->requestRestart();
		}

/*

"gt0s": 100,
            "gt1s": 100,
            "gt2s": 100,
            "gt3s": 100,
            "gt4s": 0,
            "gt5s": 0,
            "gt6s": 0,
            "gt7s": 0,
            "gt8s": 0,
            "gt0m": 100,
            "gt1m": 100,
            "gt2m": 100,
            "gt3m": 100,
            "gt4m": 0,
            "gt5m": 0,
            "gt6m": 0,
            "gt7m": 0,
            "gt8m": 0
*/


	}
	else
	{
		Serial.println(F("radar not connected"));
	}

	//m_sensor = new NewPing(m_pinTrigger, m_pinEcho, 450);
}

void ShpSensorLD2410::loop()
{
	ShpIOPort::loop();

	if (!m_radar)
		return;

	//unsigned long now = millis();
	//if (now < m_nextMeasure)
	//	return;

	if (m_radar->read())
	{
    if(m_radar->presenceDetected())
    {
      if(m_radar->stationaryTargetDetected())
      {

        Serial.print(F("Stationary target: "));
        Serial.print(m_radar->stationaryTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(m_radar->stationaryTargetEnergy());
        Serial.print(' ');

      }

      if(m_radar->movingTargetDetected())
      {
				/*
        Serial.print(F("Moving target: "));
        Serial.print(m_radar->movingTargetDistance());
        Serial.print(F("cm energy:"));
        Serial.print(m_radar->movingTargetEnergy());
				*/

				uint8_t mte = m_radar->movingTargetEnergy();

				char bar[100];
				bar[0] = 0;

				if (mte > 1)
				{
					uint8_t mteBarSize = mte / 2;
					for (int mmm = 0; mmm <= mteBarSize; mmm++)
					{
						bar [mmm] = '#';
						bar [mmm + 1] = 0;
					}

					//char buffer[100];
					//itoa(m_radar->movingTargetEnergy(), buffer, 10);
					app->publish(bar, m_valueTopic.c_str());
				}
      }
      Serial.println();
    }
    else
    {
      Serial.println(F("No target"));
    }
	}

  /*
	if (abs(m_lastDistance - distance) > m_sendDeltaFromLastDistance)
	{
		char buffer[20];
		itoa(distance, buffer, 10);
		app->publish(buffer, m_valueTopic.c_str());

		Serial.printf("Distance: %d [cm]\n", distance);
		m_lastDistance = distance;
	}
  */
}

