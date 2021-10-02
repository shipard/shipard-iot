extern Application *app;


ShpDisplay::ShpDisplay() : 
											m_queueRequests(0)
{
	for (int i = 0; i < DISPLAY_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].payload = "";
	}
}

void ShpDisplay::onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length)
{
	addQueueItem(subCmd, payload, length, 10);
}

void ShpDisplay::addQueueItem(const char *subCmd, byte* payload, unsigned int length, unsigned long startAfter)
{
	for (int i = 0; i < DISPLAY_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;	
		m_queue[i].startAfter = millis() + startAfter;

		if (subCmd)
			strcpy(m_queue[i].subCmd, subCmd);
		else
			m_queue[i].subCmd[0] = 0;

		m_queue[i].payload = "";
		for (int c = 0; c < length; c++)
			m_queue[i].payload.concat((char)payload[c]);

		m_queue[i].qState = qsDoIt;	
		m_queueRequests++;

		break;
	}
}

void ShpDisplay::loop()
{
	if (m_queueRequests == 0)
		return;

	unsigned long now = millis();

	for (int i = 0; i < DISPLAY_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsDoIt)
			continue;
		
		if (m_queue[i].startAfter > now)
			continue;

		m_queue[i].qState = qsRunning;
		runQueueItem(i);

		break;
	}	

	ShpIOPort::loop();
}

void ShpDisplay::runQueueItem(int i)
{	
	m_queueRequests--;
	m_queue[i].qState = qsFree;
}


