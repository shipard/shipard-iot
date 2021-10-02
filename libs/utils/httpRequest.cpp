extern Application *app;



ShpHttpRequest::ShpHttpRequest ()
{
}

bool ShpHttpRequest::begin(const char *url)
{
	m_httpClient.setTimeout(60);

  if (m_httpClient.begin(m_wifiClient, url)) 
  {
		int httpCode = m_httpClient.GET();
		if (httpCode > 0)
		{
			Serial.printf("[HTTP] GET... code: %d\n", httpCode);

			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
			{
			} 
			else 
			{
				Serial.printf("[HTTP] GET... failed, error: %s\n", m_httpClient.errorToString(httpCode).c_str());
				return false;
			}
		} 
		else 
		{
			Serial.printf("[HTTP] Unable to connect\n");
			return false;
		}
	}
	else
	{
		Serial.println("[HTTP] GET FAILED...");
		return false;
	}


	return true;
}

void ShpHttpRequest::end()
{
	m_httpClient.end();
}
