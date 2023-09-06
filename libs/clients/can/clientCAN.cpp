extern SHP_APP_CLASS *app;


CAN_device_t CAN_cfg;
const int rx_queue_size = 20;        // Receive Queue size


#define INT_BITS 16
uint16_t leftRotate(uint16_t n, unsigned int d)
{
  return (n << d)|(n >> (INT_BITS - d));
}

uint16_t rightRotate(uint16_t n, unsigned int d)
{
  return (n >> d)|(n << (INT_BITS - d));
}

ShpClientCAN::ShpClientCAN() :
                                m_rxPin(GPIO_NUM_17),
                                m_txPin(GPIO_NUM_16),
                                m_deviceId(0),
                                m_clientState(ccsNeedDeviceId),
                                m_clientStateRetryMillis(0),
                                m_OTAUpdate(NULL)
{
	for (int i = 0; i < CLIENT_CAN_INCOMING_QUEUE_LEN; i++)
	{
		m_incomingQueue[i].packetCmd = ROUTER_CAN_CMD_NONE;
		m_incomingQueue[i].packetType = ROUTER_CAN_SPT_NONE;
		m_incomingQueue[i].stringPacketBlockNumber = 0;
    m_incomingQueue[i].stringPacketBlocksCount = 0;
	}

	for (int i = 0; i < CLIENT_CAN_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].queueItemType = CLIENT_CAN_QIT_NONE;
    m_queue[i].deviceId = 0;
		m_queue[i].packetDataLen = 0;
	}

  WiFi.macAddress(m_clientHWId);

  for (int i = 0; i < 6; i++)
    Serial.printf("%02X:", m_clientHWId[i]);
  Serial.printf("\n");
}

void ShpClientCAN::init()
{
  //pinMode(13, OUTPUT);
  //digitalWrite(13, LOW);

  //pinMode(2, OUTPUT);
  //digitalWrite(2, HIGH);

  CAN_cfg.speed = CAN_SPEED_125KBPS;
  CAN_cfg.tx_pin_id = m_txPin;
  CAN_cfg.rx_pin_id = m_rxPin;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  CAN_cfg.tx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

	Serial.printf("CAN CLIENT START: rx: %d, tx: %d\n", m_rxPin, m_txPin);

      ESP32Can.CANStop();

      CAN_filter_t p_filter;
      p_filter.FM = Single_Mode;

      p_filter.ACR0 = 0;
      p_filter.ACR1 = 0;
      p_filter.ACR2 = 0;
      p_filter.ACR3 = 0;

      p_filter.AMR0 = 0xFF;
      p_filter.AMR1 = 0xFF;
      p_filter.AMR2 = 0xFF;
      p_filter.AMR3 = 0xFF;
      ESP32Can.CANConfigFilter(&p_filter);

  ESP32Can.CANInit();

  Serial.println("--CAN Initialized---");
}

void ShpClientCAN::doFwUpgradeRequest(String payload)
{
	Serial.println("CLIENT: FW UPGRADE REQUEST:");
	Serial.println(payload);

  if (m_OTAUpdate)
    return;

  m_OTAUpdate = new ShpOTAUpdateCAN();
  if (!m_OTAUpdate->setUpdateInfo(payload))
  {
    delete m_OTAUpdate;
    m_OTAUpdate = NULL;
    Serial.println("FWINIT: INVALID PAYLOAD");
    return;
    //m_OTAUpdate->setUpdateInfo(paload)
  }
  Serial.println("FWINIT: SUCCESS");
  //ROUTER_CAN_CMD_DEVICE_FW_UPG_REQUEST
  //publish(payload, );
  publish(payload.c_str(), NULL, ROUTER_CAN_CMD_DEVICE_FW_UPG_REQUEST);

  addDownloadFWQueueItem(0, 2000);
}

void ShpClientCAN::sendRequestDeviceId()
{
  CAN_frame_t tx_frame;

  tx_frame.FIR.B.FF = CAN_frame_std;
  tx_frame.MsgID = canMsgId_GET_DEVICE_ID;
  tx_frame.FIR.B.DLC = 6;

  memcpy(tx_frame.data.u8, m_clientHWId, CAN_DEVICE_HW_ID_LEN);

  WiFi.macAddress(m_clientHWId);

  for (int i = 0; i < 6; i++)
    Serial.printf("%02X:", m_clientHWId[i]);

  Serial.printf("sendRequestDeviceId BEGIN\n");

  int res = ESP32Can.CANWriteFrame(&tx_frame);
  Serial.printf("sendRequestDeviceId END: %d\n", res);

  m_clientState = ccsWaitForDeviceId;
  m_clientStateRetryMillis = millis() + 2 * 1000;
}

void ShpClientCAN::sendRequestCfg()
{
  Serial.println("ShpClientCAN::sendRequestCfg");

  CAN_frame_t tx_frame;

  tx_frame.FIR.B.FF = CAN_frame_std;
  tx_frame.MsgID = canMsgId_GET_DEVICE_CFG;
  tx_frame.FIR.B.DLC = 2;

  //memcpy(tx_frame.data.u8, m_clientHWId, CAN_DEVICE_HW_ID_LEN);
  tx_frame.data.u8[0] = lowByte(m_deviceId);
  tx_frame.data.u8[1] = highByte(m_deviceId);

  //Serial.printf("sendCfgRequest BEGIN\n");

  int res = ESP32Can.CANWriteFrame(&tx_frame);
  //Serial.printf("sendCfgRequest END: %d\n", res);

  m_clientState = ccsWaitForCFG;
  m_clientStateRetryMillis = millis() + 30 * 1000;
}

void ShpClientCAN::publish(const char *payload, const char *topic, const uint8_t packetCmd /* = ROUTER_CAN_CMD_PUBLISH_FROM_DEVICE */)
{
  /*
  Serial.println("ShpClientCAN::publish:");
	Serial.println(topic);
	Serial.println(payload);
	Serial.println("---");
  */

  for (int i = 0; i < CLIENT_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + 10;
		m_queue[i].queueItemType = CLIENT_CAN_QIT_PUBLISH;
		m_queue[i].deviceId = /*canMsgId_ROUTER_DEVICE*/ m_deviceId;

    if (topic)
    {
      m_queue[i].data = topic;
      m_queue[i].data.concat("\n");
    }
    m_queue[i].data += payload;

    m_queue[i].packetCmd = packetCmd;//ROUTER_CAN_CMD_PUBLISH_FROM_DEVICE;
    m_queue[i].packetType = ROUTER_CAN_SPT_STRING2;
    m_queue[i].stringPacketBlockNumber = 0;
    m_queue[i].qState = qsDoIt;

		m_queueRequests++;

		break;
	}
}

void ShpClientCAN::addDownloadFWQueueItem(uint8_t blockNumber, int startAfter)
{
  for (int i = 0; i < CLIENT_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + startAfter;
		m_queue[i].queueItemType = CLIENT_CAN_QIT_SEND_FW_BLOCK_REQUEST;
		m_queue[i].deviceId = m_deviceId;

    m_queue[i].packetCmd = ROUTER_CAN_CMD_DEVICE_FW_BLOCK_REQUEST;
    //m_queue[i].packetType = ROUTER_CAN_SPT_STRING2;
    m_queue[i].stringPacketBlockNumber = blockNumber;
    m_queue[i].qState = qsDoIt;

		m_queueRequests++;

    Serial.printf("addDownloadFWQueueItem --> block #%d\n", blockNumber);

		break;
	}
}


void ShpClientCAN::doIncomingPacket(uint32_t msgId, uint8_t dataLen, uint8_t *data)
{
  /*
  printf("Incoming packet at %010d; id `%d`, len `%d`, data: ", millis(), msgId, dataLen);
  for (int i = 0; i < dataLen; i++)
    printf("0x%02X ", data[i]);
  printf("\n");
  */

  if (msgId == canMsgId_SET_DEVICE_ID)
  {
    if (memcmp(m_clientHWId, data, CAN_DEVICE_HW_ID_LEN) == 0)
    {
      m_deviceId = word(data[CAN_DEVICE_HW_ID_LEN + 1], data[CAN_DEVICE_HW_ID_LEN]); // data[CAN_DEVICE_HW_ID_LEN] + 0xFF * data[CAN_DEVICE_HW_ID_LEN + 1];
      if (m_deviceId == 0xFFFF)
      {
        printf ("ERROR: GOT INVALID DEVICE-ID %d\n", m_deviceId);
        return;
      }
      printf ("NEW DEVICE-ID IS %d\n", m_deviceId);
      app->setHBLedStatus(hbLEDStatus_WaitForCfg);


      ESP32Can.CANStop();
      uint16_t deviceId = rightRotate(m_deviceId, 3);//((uint16_t)m_deviceId) >> 3;

      CAN_filter_t p_filter;
      p_filter.FM = Single_Mode;

      p_filter.ACR0 = lowByte(deviceId);
      p_filter.ACR1 = highByte(deviceId);
      p_filter.ACR2 = 0;
      p_filter.ACR3 = 0;

      p_filter.AMR0 = 0b00000000;
      p_filter.AMR1 = 0b00011111;
      p_filter.AMR2 = 0xFF;
      p_filter.AMR3 = 0xFF;
      ESP32Can.CANConfigFilter(&p_filter);

      delay(100);
      ESP32Can.CANInit();

      m_clientState = ccsNeedCFG;
    }
  }

  if (msgId != m_deviceId)
  {
    printf("PACKET DEVICE INVALID: msgId: %d deviceId: %d \n", msgId, m_deviceId);
    return;
  }

  int iqi = 0;

  uint8_t packetType = data[0];
  if (packetType == ROUTER_CAN_SPT_STRING2)
  {
    uint16_t blockNumber = word(data[2], data[1]);//data[1] + 0xFF * data[2];
    uint16_t stringLen = 0;
    //Serial.printf("BN: %d, %02x %02x\n", blockNumber, data[1], data[2]);
    if (blockNumber == 0)
    {
      uint8_t packetCmd = data[3];

      //int test = ;
      stringLen = word(data[5], data[4]);//data[4] + 0xFF * data[5];

      m_incomingQueue[iqi].data = "";
      m_incomingQueue[iqi].packetCmd = packetCmd;
      m_incomingQueue[iqi].packetType = packetType;
      m_incomingQueue[iqi].stringPacketBlockNumber = 0;
      m_incomingQueue[iqi].stringPacketBlocksCount = 0;
      m_incomingQueue[iqi].sendedLen = stringLen;

      Serial.printf("START STRING; strLen: %d, block: %d\n", stringLen, blockNumber);
    }
    else if (blockNumber == 0xFFFF)//(data[1] == 0xFF && data[2] == 0xFF)
    { // END
      //Serial.printf("  -> ; strLen: %d, block: %d\n", stringLen, blockNumber);
      Serial.println ("---SEND-DONE--DATA--");
      //Serial.println (m_incomingQueue[iqi].data);
      //Serial.println ("---DONE---");

      if (m_incomingQueue[iqi].packetCmd == ROUTER_CAN_CMD_DEVICE_CFG)
      {
        m_clientState = ccsRunning;
        Serial.println ("app->setIotBoxCfg");
        app->setIotBoxCfg(m_incomingQueue[iqi].data);
        app->iotBoxInfo();
      }
      else
      if (m_incomingQueue[iqi].packetCmd == ROUTER_CAN_CMD_ROUTE_TO_DEVICE)
      {
        Serial.println ("INCOMING MESSAGE:");
        Serial.println(m_incomingQueue[iqi].data);
        Serial.println ("--- END MESSAGE");

        int topicEndPos = m_incomingQueue[iqi].data.indexOf("\n");
        if (topicEndPos > 1)
        {
          String pubTopic = m_incomingQueue[iqi].data.substring(0, topicEndPos);
          topicEndPos++;
          int payloadLen = m_incomingQueue[iqi].data.length() - topicEndPos;

          Serial.println ("--- DO INCOMING MESSAGE: ");
          Serial.print ("topic: '");
          Serial.print (pubTopic);
          Serial.println ("'");
          Serial.printf ("payload: (len: %d) '", payloadLen);
          Serial.print (m_incomingQueue[iqi].data.c_str()+topicEndPos);
          Serial.println ("'");

          app->doIncomingMessage(pubTopic.c_str(), (byte*)(m_incomingQueue[iqi].data.c_str()+topicEndPos), payloadLen);
        }
      }
    }
    else
    {
      //Serial.printf("incoming block; strLen: %d, block: %d\n", stringLen, blockNumber);
      m_incomingQueue[iqi].stringPacketBlockNumber = blockNumber;
      m_incomingQueue[iqi].stringPacketBlocksCount++;

      int packetStrLen = dataLen - 3;
      for (int xx = 0; xx < packetStrLen; xx++)
      {
        m_incomingQueue[iqi].data.concat((char)data[xx + 3]);
        //Serial.print((char)data[xx + 3]);
      }
    }
  }

  if (packetType == ROUTER_CAN_SPT_FW)
  {
    m_OTAUpdate->addPacket(dataLen, data);
  }
}

void ShpClientCAN::runQueueItem(int i)
{
	if (m_queue[i].queueItemType == CLIENT_CAN_QIT_SEND_SIMPLE_PACKET)
	{
		Serial.println("--SEND SIMPLE PACKET--");
    CAN_frame_t tx_frame;

    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_queue[i].deviceId;
    tx_frame.FIR.B.DLC = m_queue[i].packetDataLen;
    memcpy(tx_frame.data.u8, m_queue[i].packetData, m_queue[i].packetDataLen);

    Serial.printf("CAN CLIENT PACKET SEND\n");
    int res = ESP32Can.CANWriteFrame(&tx_frame);
    Serial.printf("CAN CLIENT PACKET SEND done; res=%d\n", res);

  	m_queue[i].qState = qsFree;
    return;
	}

  if (m_queue[i].queueItemType == CLIENT_CAN_QIT_PUBLISH)
	{
    //Serial.println(" --> CLIENT_CAN_QIT_PUBLISH");
    m_queue[i].qState = qsInProgress;
    return;
  }

  if (m_queue[i].queueItemType == CLIENT_CAN_QIT_SEND_FW_BLOCK_REQUEST)
	{
    //Serial.println(" --> CLIENT_CAN_QIT_PUBLISH");
      //m_queue[i].qState = qsInProgress;
    m_OTAUpdate->setBlockNumber(m_queue[i].stringPacketBlockNumber);

    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_deviceId;
    tx_frame.FIR.B.DLC = 2;
    tx_frame.data.u8[0] = ROUTER_CAN_CMD_DEVICE_FW_BLOCK_REQUEST;
    tx_frame.data.u8[1] = m_queue[i].stringPacketBlockNumber;//m_OTAUpdate->m_blockNumberToDownload;
    tx_frame.data.u8[2] = 0;
    tx_frame.data.u8[3] = 0;
    tx_frame.data.u8[4] = 0;
    tx_frame.data.u8[5] = 0;
    tx_frame.data.u8[6] = 0;
    tx_frame.data.u8[7] = 0;
    int res = ESP32Can.CANWriteFrame(&tx_frame);

  	m_queue[i].qState = qsFree;

    return;
  }
}

void ShpClientCAN::doQueueItemStep(int i)
{
  if (m_queue[i].queueItemType == CLIENT_CAN_QIT_PUBLISH)
  {
    //Serial.println(" --> CLIENT_CAN_QIT_PUBLISH");
    if (m_queue[i].stringPacketBlockNumber == 0)
    { // START packet
      int strLen = m_queue[i].data.length();
      m_queue[i].stringPacketBlocksCount = int(strLen / CAN_SEND_STRING_PACKET_DATA_LEN);
      if (strLen % CAN_SEND_STRING_PACKET_DATA_LEN != 0)
        m_queue[i].stringPacketBlocksCount++;

      //printf ("START STRING SEND; total packets count: %d\n", m_queue[i].stringPacketBlocksCount);

      CAN_frame_t tx_frame;
      tx_frame.FIR.B.FF = CAN_frame_std;
      tx_frame.MsgID = m_queue[i].deviceId;
      tx_frame.FIR.B.DLC = 6;
      tx_frame.data.u8[0] = m_queue[i].packetType;
      tx_frame.data.u8[1] = 0;
      tx_frame.data.u8[2] = 0;
      tx_frame.data.u8[3] = m_queue[i].packetCmd;
      tx_frame.data.u8[4] = lowByte(strLen);
      tx_frame.data.u8[5] = highByte(strLen);
      int res = ESP32Can.CANWriteFrame(&tx_frame);

      m_queue[i].stringPacketBlockNumber++;
      return;
    }

    if (m_queue[i].stringPacketBlockNumber == m_queue[i].stringPacketBlocksCount + 1)
    { // END packet
      //printf ("END STRING SEND\n");

      CAN_frame_t tx_frame;
      tx_frame.FIR.B.FF = CAN_frame_std;
      tx_frame.MsgID = m_queue[i].deviceId;
      tx_frame.FIR.B.DLC = 5;
      tx_frame.data.u8[0] = m_queue[i].packetType;
      tx_frame.data.u8[1] = lowByte(0xFFFF);
      tx_frame.data.u8[2] = highByte(0xFFFF);
      tx_frame.data.u8[3] = 0;
      tx_frame.data.u8[4] = 0;
      int res = ESP32Can.CANWriteFrame(&tx_frame);

      m_queue[i].data = "";
      m_queue[i].qState = qsFree;
      return;
    }

    int strPos = (m_queue[i].stringPacketBlockNumber - 1) * CAN_SEND_STRING_PACKET_DATA_LEN;
    int strLen = CAN_SEND_STRING_PACKET_DATA_LEN;
    if (strPos + strLen > m_queue[i].data.length())
      strLen = m_queue[i].data.length() - strPos;

    //Serial.printf ("  -> SEND PACKET; #%d, from: %06d, len: %d, data: '", m_queue[i].stringPacketBlockNumber, strPos, strLen);

    //for (int xx = 0; xx < strLen; xx++)
    //  Serial.print(m_queue[i].data.c_str()[strPos + xx]);

    //Serial.printf("'\n");

    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_queue[i].deviceId;
    tx_frame.FIR.B.DLC = 3 + strLen;
    tx_frame.data.u8[0] = m_queue[i].packetType;
    tx_frame.data.u8[1] = lowByte(m_queue[i].stringPacketBlockNumber);
    tx_frame.data.u8[2] = highByte(m_queue[i].stringPacketBlockNumber);
    for (int xx = 0; xx < strLen; xx++)
      tx_frame.data.u8[3 + xx] = m_queue[i].data.c_str()[strPos + xx];

    int res = ESP32Can.CANWriteFrame(&tx_frame);
    //Serial.println(res);
    delay(1);

    /*
    printf("send string packet %04d; len `%d`, data: ", m_queue[i].stringPacketBlockNumber, tx_frame.FIR.B.DLC);
    for (int ixx = 0; ixx < 8; ixx++)
    {
      if (ixx < 3 || tx_frame.data.u8[ixx] < 32 || tx_frame.data.u8[ixx] > 126)
        printf("0x%02X ", tx_frame.data.u8[ixx]);
      else
        printf("  %c  ", tx_frame.data.u8[ixx]);
    }
    printf("\n");
    */

    //delay(5);

    m_queue[i].stringPacketBlockNumber++;
  }
}

void ShpClientCAN::loop()
{
  CAN_frame_t rx_frame;
  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      //printf("New standard frame");
    }
    else {
      //printf("New extended frame");
    }

    if (rx_frame.FIR.B.RTR == CAN_RTR) {
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    }
    else {
      doIncomingPacket(rx_frame.MsgID, rx_frame.FIR.B.DLC, rx_frame.data.u8);
      return;
    }
  }

  unsigned long currentMillis = millis();

  if (m_clientState != ccsRunning)
  {
    if (currentMillis > m_clientStateRetryMillis)
    {
      Serial.printf("TEST-RETRY: %d\n", m_clientState);
      if (m_clientState == ccsWaitForDeviceId)
      {
        Serial.println("RETRY: m_clientState = ccsNeedDeviceId");
        m_clientState = ccsNeedDeviceId;
      }
      else if (m_clientState == ccsWaitForCFG)
      {
        Serial.println("RETRY: m_clientState = ccsNeedCFG;");
        m_clientState = ccsNeedCFG;
      }
    }

    if (m_clientState == ccsNeedDeviceId)
    {
      sendRequestDeviceId();
      return;
    }
    if (m_clientState == ccsNeedCFG)
    {
      sendRequestCfg();
      return;
    }

    return;
  }

  unsigned long now = millis();
	for (int i = 0; i < CLIENT_CAN_CONTROL_QUEUE_LEN; i++)
	{
    if (m_queue[i].qState == qsInProgress)
    {
      doQueueItemStep(i);
      break;
    }

		if (m_queue[i].qState != qsDoIt)
			continue;

		if (m_queue[i].startAfter > now)
			continue;

		m_queue[i].qState = qsRunning;
		runQueueItem(i);

		break;
	}

  if (m_OTAUpdate && m_OTAUpdate->m_phase != OTA_SLOW_PHASE_INCOMING_PACKETS)
  {
    uint8_t nextOp = m_OTAUpdate->checkUpdateProgress();
    if (nextOp == OTA_SLOW_NEXT_OP_NEXT_BLOCK)
    {
      addDownloadFWQueueItem(m_OTAUpdate->m_blockNumberToDownload, 300);
      m_OTAUpdate->m_phase = OTA_SLOW_PHASE_INCOMING_PACKETS;
    }
  }
}
