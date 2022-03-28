extern SHP_APP_CLASS *app;



ShpTelnet::ShpTelnet(int listenPort) :
																m_listenPort(listenPort),
																m_server(NULL)
{
	m_server = new WiFiServer(m_listenPort);
	m_server->begin();
  m_server->setNoDelay(true);
}

void ShpTelnet::write(char c)
{
	for (int i = 0; i < MAX_SHP_TELNET_CLIENTS; i++)
	{
		if (m_serverClients[i] && m_serverClients[i].connected())
		{
    	m_serverClients[i].write(c);
			m_serverClients[i].flush();
      delay(0);
    }
  }
}

void ShpTelnet::write(char *string)
{
	for (int i = 0; i < MAX_SHP_TELNET_CLIENTS; i++)
	{
		if (m_serverClients[i] && m_serverClients[i].connected())
		{
			
    	m_serverClients[i].write(string);
			m_serverClients[i].flush();
      delay(0);
    }
  }
}

void ShpTelnet::loop(Stream *readStream, Stream *writeStream)
{
	uint8_t i;

	if (m_server->hasClient())
	{
		for (i = 0; i < MAX_SHP_TELNET_CLIENTS; i++)
		{
    	if (!m_serverClients[i] || !m_serverClients[i].connected())
			{
				if(m_serverClients[i])
					m_serverClients[i].stop();

				m_serverClients[i] = m_server->available();

				/*
				if (!m_serverClients[i]) 
					Serial.println("available broken");
				Serial.print("New client: ");
				Serial.print(i); 
				Serial.print(' ');
				Serial.println(m_serverClients[i].remoteIP());
				*/

				m_serverClients[i].setNoDelay(true);
      	m_serverClients[i].flush();
				//m_serverClients[i].write("hello!\n", 7);
				//m_serverClients[i].flush();
				
				break;
      }
 		}
	}

	if (readStream)
	{
		int sbCnt = 0;
		while (readStream->available()) 
		{
			char c = (char)readStream->read();
		
			m_buffer[sbCnt] = c;
			m_buffer[sbCnt + 1] = 0;

			sbCnt++;
			if (sbCnt == MAX_DATA_TELNET_BUF_LEN)
			{
				write(m_buffer);
				sbCnt = 0;
				m_buffer[0] = 0;
			}
		}

		if (sbCnt)
		{
			write(m_buffer);
			sbCnt = 0;
			m_buffer[0] = 0;
		}
	}

	for(i = 0; i < MAX_SHP_TELNET_CLIENTS; i++)
	{
		if (m_serverClients[i] && m_serverClients[i].connected())
		{
			if(m_serverClients[i].available())
			{
				while(m_serverClients[i].available())
				{
					int data = m_serverClients[i].read();
					if (data == 13)
						continue;

					writeStream->write(data);
				}
			}
		}
		else 
		{
			if (m_serverClients[i]) 
			{
				m_serverClients[i].stop();
			}
		}
	}	

	yield();
}
