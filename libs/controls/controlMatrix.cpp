extern SHP_APP_CLASS *app;

#define STATE_ROWS_ON HIGH
#define STATE_ROWS_OFF LOW

#define STATE_COLS_ON HIGH
#define STATE_COLS_OFF LOW


ShpControlMatrix::ShpControlMatrix() :
                                        m_cntCols (0),
                                        m_cntRows (0),
																				m_pinStop(-1),
                                        m_pinCurrent(-1),
                                        m_isBusy(false),
                                        m_activeCol(-1),
                                        m_activeRow(-1),
                                        m_needStop(0),
                                        m_stopTimeout(10000),
                                        m_stopAfter(0),
                                        m_needSendBusy(false)
{
}

void ShpControlMatrix::init(JsonVariant portCfg)
{
	/* control/matrix config format:
	 * ----------------------------
    {
      "type": "control/matrix",
      "portId": "M",
      "pinStop": 21,
      "pinCurrent": 1,
      "pinsCols": [2, 4, 5, 6, 7, 8],
      "pinsRows": [9, 10, 11, 12, 13, 14, 15, 16]
    }
	-------------------------------*/

	ShpIOPort::init(portCfg);

	// -- pinStop
	if (portCfg.containsKey("pinStop"))
		m_pinStop = portCfg["pinStop"];

	if (portCfg.containsKey("pinCurrent"))
		m_pinCurrent = portCfg["pinCurrent"];

	if (portCfg.containsKey("pinsCols"))
  {
  	JsonArray pins = portCfg["pinsCols"];
    for (JsonVariant onePin: pins)
    {
      m_pinsCols[m_cntCols] = onePin;
      m_cntCols++;
    }
  }
	if (portCfg.containsKey("pinsRows"))
  {
  	JsonArray pins = portCfg["pinsRows"];
    for (JsonVariant onePin: pins)
    {
      m_pinsRows[m_cntRows] = onePin;
      m_cntRows++;
    }
  }

	m_topicBusy = "shp/sensors/" + app->m_deviceId + "/" + m_portId + "/" + "busy";
}

void ShpControlMatrix::init2()
{
  Serial.println("COLS:");
  for (int i = 0; i < m_cntCols; i++)
  {
    Serial.println(m_pinsCols[i]);
    pinMode(m_pinsCols[i], OUTPUT);
    digitalWrite(m_pinsCols[i], STATE_COLS_OFF);
  }

  Serial.println("ROWS:");
  for (int i = 0; i < m_cntRows; i++)
  {
    Serial.println(m_pinsRows[i]);
    pinMode(m_pinsRows[i], OUTPUT);
    digitalWrite(m_pinsRows[i], STATE_ROWS_OFF);
  }

  pinMode(m_pinStop, INPUT);
  attachInterrupt(m_pinStop, std::bind(&ShpControlMatrix::onStop, this, m_pinStop), FALLING);
}

void ShpControlMatrix::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
  /* PAYLOADS:
	 * "XY" / "12" -> ON col X / col Y
	 * "0"         -> OFF
	 */

	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	if (strcmp(payloadStr.c_str(), "0") == 0)
	{
    Serial.println("---OFF---");
    matrixOff();
    return;
	}

  if (length == 2)
  {
    uint8_t col = payload[0] - '0';
    uint8_t row = payload[1] - '0';
    Serial.println("---ON---");
    Serial.println(col);
    Serial.println(row);

    matrixOn(col, row);
  }
}

void ShpControlMatrix::onStop(int pin)
{
	m_needStop++;
}

void ShpControlMatrix::matrixOn(uint8_t col, uint8_t row)
{
  if (m_isBusy)
    return;

  m_isBusy = true;
  m_needSendBusy = true;
  m_activeCol = col;
  m_activeRow = row;
  m_stopAfter = millis() + m_stopTimeout;

  for (int i = 0; i < m_cntCols; i++)
  {
    digitalWrite(m_pinsCols[i], STATE_COLS_OFF);
  }

  Serial.println("ROWS:");
  for (int i = 0; i < m_cntRows; i++)
  {
    digitalWrite(m_pinsRows[i], STATE_ROWS_OFF);
  }

  Serial.println("---ON PINS---");
  Serial.println(m_pinsCols[m_activeCol]);
  Serial.println(m_pinsRows[m_activeRow]);

  digitalWrite(m_pinsCols[m_activeCol], STATE_COLS_ON);
  digitalWrite(m_pinsRows[m_activeRow], STATE_ROWS_ON);
}

void ShpControlMatrix::matrixOff()
{
  Serial.println("___MATRIX_OFF__");
  Serial.println(m_activeCol);
  Serial.println(m_activeRow);

  for (int i = 0; i < m_cntCols; i++)
  {
    digitalWrite(m_pinsCols[i], STATE_COLS_OFF);
  }

  Serial.println("ROWS:");
  for (int i = 0; i < m_cntRows; i++)
  {
    digitalWrite(m_pinsRows[i], STATE_ROWS_OFF);
  }

  m_isBusy = false;
  m_needSendBusy = true;
  m_activeCol = -1;
  m_activeRow = -1;
  m_needStop = 0;
  m_stopAfter = 0;
}

void ShpControlMatrix::loop()
{
  if (m_needStop > 1)
  {
    matrixOff();
  }

  if (m_isBusy && m_stopAfter && m_stopAfter < millis())
  {
    matrixOff();
  }

  if (m_needSendBusy)
  {
    app->publish(m_isBusy ? "1" : "0", m_topicBusy.c_str());
    m_needSendBusy = false;
  }

  //ShpIOPort::loop();
}

