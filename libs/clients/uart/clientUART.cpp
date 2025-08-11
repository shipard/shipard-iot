extern SHP_APP_CLASS *app;


ShpClientUART::ShpClientUART() : m_rowAppended(0)
{
}


void ShpClientUART::loop()
{
  HardwareSerial *m_hwSerial = &Serial;

	while (m_hwSerial->available())
	{
		char c = (char)m_hwSerial->read();
    //Serial.print(c);
		if (c == '\r')
      continue;

    if (c == '\n')
		{
			m_rowAppended = 1;
			break;
		}
    m_oneRowBuffer.concat(c);
	}

  if (m_rowAppended)
  {
    if (m_oneRowBuffer.startsWith(">>>"))
    {
      String topic;
      String cmdValue;

      int valuePos = m_oneRowBuffer.indexOf(' ');
      if (valuePos != -1)
      {
        cmdValue = m_oneRowBuffer.c_str() + valuePos + 1;
        topic = m_oneRowBuffer.substring(3, valuePos);
      }
      else
        topic = m_oneRowBuffer.substring(3);

      app->doIncomingMessage(topic.c_str(), (byte*)cmdValue.c_str(), cmdValue.length(), 1);
    }

    m_oneRowBuffer = "";
    m_rowAppended = 0;
  }
}

