extern Application *app;



ShpSensorDistanceUS::ShpSensorDistanceUS() : 
																m_pinTrigger(-1), 
																m_pinEcho(-1),
																m_measureInterval(1000),
																m_cntShots(5),
																m_shotInterval(40),
																m_lastDistance(-1),
																m_sendDeltaFromLastDistance(3),
																m_nextMeasure(0),
																m_sensor(nullptr)
{
}

void ShpSensorDistanceUS::init(JsonVariant portCfg)
{
	/* ultrasonic distance sensor config format:
	 * -----------------------------------------
	 {
			"type": "sensorDistanceUS",
			"portId": "uio-5-1",
			"pinTrigger": 16,
			"pinEcho": 17
		}
	---------------------------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members


	// -- rx / tx pin
	if (portCfg["pinTrigger"] != nullptr)
		m_pinTrigger = portCfg["pinTrigger"];
	if (portCfg["pinEcho"] != nullptr)
		m_pinEcho = portCfg["pinEcho"];

	if (m_pinTrigger < 0 || m_pinEcho < 0)
		return;

	m_sensor = new NewPing(m_pinTrigger, m_pinEcho, 450);
}

void ShpSensorDistanceUS::loop()
{
	ShpIOPort::loop();

	if (!m_sensor)
		return;

	unsigned long now = millis();
	if (now < m_nextMeasure)
		return;

	int sumDistance = 0;
	for (int i = 0; i < m_cntShots; i++)
	{
		int d = m_sensor->ping_cm();
		sumDistance += d;
		delay(m_shotInterval);
	}

	int distance = sumDistance / m_cntShots;
		
	if (abs(m_lastDistance - distance) > m_sendDeltaFromLastDistance)
	{
		char buffer[20];
		itoa(distance, buffer, 10);
		app->publish(buffer, m_valueTopic.c_str());

		Serial.printf("Distance: %d [cm]\n", distance);
		m_lastDistance = distance;
	}
}

