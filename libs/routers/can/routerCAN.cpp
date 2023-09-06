extern SHP_APP_CLASS *app;

void data2HexStr(uint8_t* data, size_t len, char str[]);

CAN_device_t CAN_cfg;


ShpRouterCAN::ShpRouterCAN() :
                            m_rxPin(GPIO_NUM_33),
                            m_txPin(GPIO_NUM_32),
                            m_deviceMsgId(canMsgId_ROUTER_DEVICE),
														m_queueRequests(0),
                            m_devicesCount(0),
                            m_maxDevicesCount(10),
                            m_devices(nullptr)
{
	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_NONE;
    m_queue[i].deviceId = 0;
		m_queue[i].packetDataLen = 0;
    m_queue[i].nextRunAfter = 0;
	}
}

void ShpRouterCAN::init(JsonVariant portCfg)
{
	ShpIOPort::init(portCfg);

	// -- rx / tx pin / control

	if (portCfg.containsKey("pinRX"))
		m_rxPin = portCfg["pinRX"];
	if (portCfg.containsKey("pinTX"))
		m_txPin = portCfg["pinTX"];

	if (m_txPin < 0 || m_rxPin < 0)
		return;

	if (portCfg.containsKey("rt"))
  {
	  JsonArray routedTopics = portCfg["rt"];
    m_maxDevicesCount = routedTopics.size();
    m_devicesCount = m_maxDevicesCount;
    m_devices = new DeviceItem[m_maxDevicesCount];
    m_incomingQueue = new IncomingQueueItem[m_maxDevicesCount];
    for (int i = 0; i < m_maxDevicesCount; i++)
    {
      m_incomingQueue[i].packetCmd = 0;
      m_incomingQueue[i].packetType = 0;
      m_incomingQueue[i].stringPacketBlockNumber = 0;
      m_incomingQueue[i].stringPacketBlocksCount = 0;
      m_incomingQueue[i].sendedLen = 0;
      m_incomingQueue[i].data = "";
    }

    int deviceNdx = 0;
    for (JsonVariant oneRoutedTopic: routedTopics)
    {
      const char* topic = oneRoutedTopic["t"];
      const char* hwId = oneRoutedTopic["m"];

      m_devices[deviceNdx].otaSender = NULL;
      m_devices[deviceNdx].topic = topic;
      char* ptr;
      m_devices[deviceNdx].clientHWId[0] = strtol(hwId, &ptr, HEX );
      for( uint8_t mi = 1; mi < 6; mi++ )
        m_devices[deviceNdx].clientHWId[mi] = strtol(ptr+1, &ptr, HEX );

      deviceNdx++;
      app->m_routedTopicsCount++;
    }
  }

  const int rx_queue_size = 30;

  CAN_cfg.speed = CAN_SPEED_125KBPS;
  CAN_cfg.tx_pin_id = m_txPin;
  CAN_cfg.rx_pin_id = m_rxPin;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  CAN_cfg.tx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

	Serial.printf("CAN ROUTER START: rx: %d, tx: %d\n", m_rxPin, m_txPin);

	m_valueTopic = MQTT_TOPIC_THIS_DEVICE "/";
	m_valueTopic.concat(app->m_deviceId);
	m_valueTopic.concat("/");
	m_valueTopic.concat(m_portId);

  ESP32Can.CANInit();

  Serial.println("--CAN Initialized---");
}

void ShpRouterCAN::init2()
{
  for (int i = 0; i < m_maxDevicesCount; i++)
  {
    app->subscribeIOPortTopic (m_appPortIndex, m_devices[i].topic);
  }
}

void ShpRouterCAN::addQueueItem_SendSimplePacket(int deviceId, int8_t packetDataLen, const byte* packetData, unsigned long startAfter)
{
	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + startAfter;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_SEND_SIMPLE_PACKET;
		m_queue[i].deviceId = deviceId;
		m_queue[i].packetDataLen = packetDataLen;
    memcpy(m_queue[i].packetData, packetData, packetDataLen);

		m_queue[i].qState = qsDoIt;
    m_queue[i].nextRunAfter = 0;
		m_queueRequests++;

		break;
	}
}

void ShpRouterCAN::addQueueItem_DeviceCfgRequest(int deviceId, unsigned long startAfter)
{
	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + startAfter;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_GET_DEVICE_CFG;
		m_queue[i].deviceId = deviceId;
		m_queue[i].packetDataLen = 0;
    m_queue[i].nextRunAfter = 0;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpRouterCAN::addQueueItem_FWUpgradeRequest(int deviceId, const char *payload)
{
	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + 300;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_UPLOAD_FIRMWARE_BEGIN;
		m_queue[i].deviceId = deviceId;
		m_queue[i].packetDataLen = 0;
    m_queue[i].data = payload;
    m_queue[i].nextRunAfter = 0;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpRouterCAN::addQueueItem_FWUpgradeSendBlock(int deviceId, uint8_t blockNumber)
{
	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + 300;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_UPLOAD_FIRMWARE_SEND_BLOCK;
		m_queue[i].deviceId = deviceId;
		m_queue[i].packetDataLen = blockNumber;
    m_queue[i].data = "";
    m_queue[i].nextRunAfter = 0;

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}


void ShpRouterCAN::runQueueItem(int i)
{
	//m_queueRequests--;

	if (m_queue[i].queueItemType == ROUTER_CAN_QIT_SEND_SIMPLE_PACKET)
	{
		Serial.println("--SEND SIMPLE PACKET--");
    CAN_frame_t tx_frame;

    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_queue[i].deviceId;
    tx_frame.FIR.B.DLC = m_queue[i].packetDataLen;
    memcpy(tx_frame.data.u8, m_queue[i].packetData, m_queue[i].packetDataLen);

    Serial.printf("CAN ROUTER PACKET SEND\n");
    int res = ESP32Can.CANWriteFrame(&tx_frame);
    Serial.printf("CAN ROUTER PACKET SEND done; res=%d\n", res);

  	m_queue[i].qState = qsFree;
    return;
	}

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_GET_DEVICE_CFG)
	{
    int deviceNdx = m_queue[i].deviceId - ROUTER_CAN_FIRST_DEVICE_ID;
    if (deviceNdx < 0 || deviceNdx >= m_maxDevicesCount)
    {
      printf("INVALID DEVICE NDX %d\n", deviceNdx);
      m_queue[i].qState = qsFree;
      return;
    }

    //String deviceCfgData;

    m_queue[i].data = "";
    if (app->getDeviceCfg(m_devices[deviceNdx].clientHWId, m_queue[i].data) > 0)
    {
      //Serial.println(data.length());
      //printf ("DEVICE CFG LOADED, len=%d\n", m_queue[i].data.length());
      //Serial.println(m_queue[i].data);

      m_queue[i].queueItemType = ROUTER_CAN_QIT_SEND_STRING_DATA;
      m_queue[i].packetCmd = ROUTER_CAN_CMD_DEVICE_CFG;
      m_queue[i].packetType = ROUTER_CAN_SPT_STRING2;
      m_queue[i].stringPacketBlockNumber = 0;
      m_queue[i].qState = qsDoIt;

      return;
    }

    m_queue[i].qState = qsFree;
    return;
  }

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_SEND_STRING_DATA)
	{
    m_queue[i].qState = qsInProgress;
    return;
  }

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_UPLOAD_FIRMWARE_BEGIN)
	{
    int deviceNdx = m_queue[i].deviceId - ROUTER_CAN_FIRST_DEVICE_ID;
    if (!m_devices[deviceNdx].otaSender)
    {
      m_devices[deviceNdx].otaSender = new ShpOTAUpdateSlowSender();
      if (!m_devices[deviceNdx].otaSender->setUpdateInfo(m_queue[i].data.c_str(), 4))
      {
        delete m_devices[deviceNdx].otaSender;
        m_devices[deviceNdx].otaSender = NULL;
        m_queue[i].qState = qsFree;
        Serial.println("UPLOAD FW REQUEST ERROR");
        return;
      }
    }
    Serial.printf("UPLOAD FW REQUEST ACCEPTED, deviceId: %d, deviceNdx: %d\n", m_queue[i].deviceId, deviceNdx);

    m_queue[i].qState = qsFree;

    return;
  }

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_UPLOAD_FIRMWARE_SEND_BLOCK)
	{
    int deviceNdx = m_queue[i].deviceId - ROUTER_CAN_FIRST_DEVICE_ID;
    m_devices[deviceNdx].otaSender->getBlock(m_queue[i].packetDataLen);

    m_queue[i].qState = qsInProgress;
    return;
  }
}

uint8_t ShpRouterCAN::doQueueItemStep(int i)
{
  if (m_queue[i].nextRunAfter > millis())
  {
    return 0;
  }

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_SEND_STRING_DATA)
  {
    if (m_queue[i].stringPacketBlockNumber == 0)
    { // START packet
      int strLen = m_queue[i].data.length();
      m_queue[i].stringPacketBlocksCount = int(strLen / CAN_SEND_STRING_PACKET_DATA_LEN);
      if (strLen % CAN_SEND_STRING_PACKET_DATA_LEN != 0)
        m_queue[i].stringPacketBlocksCount++;

      printf ("START STRING SEND; total packets count: %d\n", m_queue[i].stringPacketBlocksCount);

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
      return 1;
    }

    if (m_queue[i].stringPacketBlockNumber == m_queue[i].stringPacketBlocksCount + 1)
    { // END packet
      printf ("END STRING SEND\n");

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
      return 1;
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

    m_queue[i].stringPacketBlockNumber++;
    return 1;
  }

  if (m_queue[i].queueItemType == ROUTER_CAN_QIT_UPLOAD_FIRMWARE_SEND_BLOCK)
  {
    int deviceNdx = m_queue[i].deviceId - ROUTER_CAN_FIRST_DEVICE_ID;
    uint8_t packetData[4];
    int packetDataLen = m_devices[deviceNdx].otaSender->getNextPacket(packetData);

    if (packetDataLen == 0)
    {
      m_queue[i].qState = qsFree;
      return 0;
    }

    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_queue[i].deviceId;
    tx_frame.FIR.B.DLC = 4 + packetDataLen;
    tx_frame.data.u8[0] = ROUTER_CAN_SPT_FW;
    tx_frame.data.u8[1] = m_devices[deviceNdx].otaSender->m_blockNumber;
    tx_frame.data.u8[2] = lowByte(m_devices[deviceNdx].otaSender->m_packetInBlockNumber);
    tx_frame.data.u8[3] = highByte(m_devices[deviceNdx].otaSender->m_packetInBlockNumber);
    tx_frame.data.u8[4] = packetData[0];
    tx_frame.data.u8[5] = packetData[1];
    tx_frame.data.u8[6] = packetData[2];
    tx_frame.data.u8[7] = packetData[3];

    int res = ESP32Can.CANWriteFrame(&tx_frame);

    return 0;

/* END
    uint8_t packetData[4];
    int nextPacketSize = m_queue[i].otaSender->getNextPacket(packetData);
    if (nextPacketSize == 0)
    {
      CAN_frame_t tx_frame;
      tx_frame.FIR.B.FF = CAN_frame_std;
      tx_frame.MsgID = m_queue[i].deviceId;
      tx_frame.FIR.B.DLC = 4 + 4;
      tx_frame.data.u8[0] = ROUTER_CAN_SPT_FW;
      tx_frame.data.u8[1] = 0xFE;
      tx_frame.data.u8[2] = 0xFE;
      tx_frame.data.u8[3] = 0xFE;
      tx_frame.data.u8[4] = 0;
      tx_frame.data.u8[5] = 0;
      tx_frame.data.u8[6] = 0;
      tx_frame.data.u8[4] = 0;
      int res = ESP32Can.CANWriteFrame(&tx_frame);
      delay(10);

      Serial.println ("DONE DONE DONE!");
      delete m_queue[i].otaSender;
      m_queue[i].otaSender = NULL;
      m_queue[i].qState = qsFree;
      return 1;
    }
*/

    /* SEND PACKET
    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = m_queue[i].deviceId;
    tx_frame.FIR.B.DLC = 4 + nextPacketSize;
    tx_frame.data.u8[0] = ROUTER_CAN_SPT_FW;
    tx_frame.data.u8[1] = m_queue[i].otaSender->m_blockNumber;
    tx_frame.data.u8[2] = lowByte(m_queue[i].otaSender->m_packetInBlockNumber);
    tx_frame.data.u8[3] = highByte(m_queue[i].otaSender->m_packetInBlockNumber);
    tx_frame.data.u8[4] = packetData[0];
    tx_frame.data.u8[5] = packetData[1];
    tx_frame.data.u8[6] = packetData[2];
    tx_frame.data.u8[7] = packetData[3];

    int res = ESP32Can.CANWriteFrame(&tx_frame);

    if ((m_queue[i].otaSender->m_sendedBytes % 3840) == 0)
    {
      Serial.print(">");
      m_queue[i].nextRunAfter = millis() + 2000;
    }
    else
     if ((m_queue[i].otaSender->m_sendedBytes % 512) == 0)
      m_queue[i].nextRunAfter = millis() + 50;
    else if ((m_queue[i].otaSender->m_sendedBytes % 64) == 0)
      m_queue[i].nextRunAfter = millis() + 10;
    //else
    //  m_queue[i].nextRunAfter = millis() + 3;

    return 0;
    */
  }

  return 1;
}

void ShpRouterCAN::doIncomingPacket(uint32_t msgId, uint8_t dataLen, uint8_t *data)
{

  printf("Incoming packet id `%d`, len `%d`, data: ", msgId, dataLen);
  for (int i = 0; i < dataLen; i++)
  {
    if (i < 3 || data[i] < 32 || data[i] > 126)
      printf("0x%02X ", data[i]);
    else
      printf("  %c  ", data[i]);
  }
  printf("\n");


  if (msgId == canMsgId_GET_DEVICE_ID)
  {
    printf ("  --> get device ID for ");
    for (int i = 0; i < dataLen; i++)
    {
      if (i)
        printf("-");
      printf("%02X", data[i]);
    }
    printf("\n");

    int deviceId = getDeviceId(data);
    printf("New deviceId is %d\n", deviceId);

    uint8_t packet[ROUTER_CAN_STD_PACKET_LEN];
    memcpy(packet, data, CAN_DEVICE_HW_ID_LEN);

    packet[CAN_DEVICE_HW_ID_LEN] = lowByte(deviceId);
    packet[CAN_DEVICE_HW_ID_LEN + 1] = highByte(deviceId);

    addQueueItem_SendSimplePacket(canMsgId_SET_DEVICE_ID, CAN_DEVICE_HW_ID_LEN + 2, packet, 5);

    return;
  }

  if (msgId == canMsgId_GET_DEVICE_CFG)
  {
    printf ("  --> get device cfg request: ");
    for (int i = 0; i < dataLen; i++)
    {
      if (i)
        printf("-");
      printf("%02X", data[i]);
    }
    printf("\n");

    int deviceId = data[0] + 0xFF * data[1];
    int deviceNdx = deviceId - ROUTER_CAN_FIRST_DEVICE_ID;
    if (m_devices[deviceNdx].otaSender)
    {
      delete m_devices[deviceNdx].otaSender;
      m_devices[deviceNdx].otaSender = NULL;
    }

    printf("CFG deviceId is %d\n", deviceId);
    addQueueItem_DeviceCfgRequest(deviceId, 100);

    return;
  }

  int deviceNdx = msgId - ROUTER_CAN_FIRST_DEVICE_ID;
  if (deviceNdx < 0 || deviceNdx > m_maxDevicesCount)
  {
    Serial.println("ERROR: invalid device ndx");

    return;
  }

  int iqi = deviceNdx;

  uint8_t packetType = data[0];

  if (packetType == ROUTER_CAN_CMD_DEVICE_FW_BLOCK_REQUEST)
  {
    Serial.printf("ROUTER_CAN_CMD_DEVICE_FW_BLOCK_REQUEST: %d, deviceNdx: %d\n", data[1], deviceNdx);
    addQueueItem_FWUpgradeSendBlock(msgId, data[1]);
    return;
  }

  uint16_t blockNumber = word(data[2], data[1]);
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

    //Serial.printf("START STRING; strLen: %d, block: %d\n", stringLen, blockNumber);
  }
  else if (blockNumber == 0xFFFF)//(data[1] == 0xFF && data[2] == 0xFF)
  { // END
    //Serial.printf("  -> ; strLen: %d, block: %d\n", stringLen, blockNumber);
    Serial.println ("---SUB-DONE--DATA--");
    Serial.println (m_incomingQueue[iqi].data);
    Serial.println ("---DONE---");

    if (m_incomingQueue[iqi].packetCmd == ROUTER_CAN_CMD_PUBLISH_FROM_DEVICE)
    {
      Serial.println ("app->publish");

      int topicEndPos = m_incomingQueue[iqi].data.indexOf("\n");
      if (topicEndPos > 1)
      {
        String pubTopic = m_incomingQueue[iqi].data.substring(0, topicEndPos);
        Serial.print ("topic: '");
        Serial.print (pubTopic);
        Serial.println ("'");


        topicEndPos++;

        Serial.print ("payload: '");
        Serial.print (m_incomingQueue[iqi].data.c_str()+topicEndPos);
        Serial.println ("'");

        app->publish(m_incomingQueue[iqi].data.c_str()+topicEndPos, pubTopic.c_str());
      }
    }
    else if (m_incomingQueue[iqi].packetCmd == ROUTER_CAN_CMD_DEVICE_FW_UPG_REQUEST)
    {
      Serial.println("FW UPGRADE REQUEST 2");
      addQueueItem_FWUpgradeRequest(msgId, m_incomingQueue[iqi].data.c_str());
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

int ShpRouterCAN::searchDeviceNdxByHWId(const uint8_t *hwId)
{
  // CAN_DEVICE_HW_ID_LEN

  for (int i = 0; i < m_devicesCount; i++)
  {
    if (memcmp(hwId, m_devices[i].clientHWId, CAN_DEVICE_HW_ID_LEN) == 0)
    {
      printf("   --> device found, ndx = %d\n", i);
      return i;
    }
  }

  /*
  memcpy(m_devices[m_devicesCount].clientHWId, hwId, CAN_DEVICE_HW_ID_LEN);
  m_devicesCount++;
  printf("   --> device added, ndx = %d\n", m_devicesCount);
  return m_devicesCount - 1;
  */

  printf("   --> device NOT found, ERROR!!!\n");

  return 0xFFFF;
}

int ShpRouterCAN::getDeviceId(const uint8_t *hwId)
{
  int deviceNdx = searchDeviceNdxByHWId(hwId);
  if (deviceNdx == 0xFFFF)
    return 0xFFFF;

  return deviceNdx + ROUTER_CAN_FIRST_DEVICE_ID;
}

void ShpRouterCAN::loop()
{
  //unsigned long currentMillis = millis();


  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    /*
    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      printf("New standard frame");
    }
    else {
      printf("New extended frame");
    }
    */

    if (rx_frame.FIR.B.RTR == CAN_RTR)
    {
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    }
    else
    {
      doIncomingPacket(rx_frame.MsgID, rx_frame.FIR.B.DLC, rx_frame.data.u8);
    }
  }

	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
    if (m_queue[i].qState == qsInProgress)
    {
      if (doQueueItemStep(i))
        return;
    }
  }

  unsigned long now = millis();

	for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
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

void ShpRouterCAN::onMessage(byte* payload, unsigned int length, const char* subCmd)
{

  Serial.println("ShpRouterCAN::onMessage");
  //Serial.println(topic);
  Serial.println("---------");

  //routeMessage(topic, subCmd, payload, length);
}


void ShpRouterCAN::routeMessage(const char* topic, byte* payload, unsigned int length)
{
  Serial.println("ShpClientCAN::routeMessage:");
	Serial.println(topic);
	//Serial.println(payload);
	Serial.println("---");

  int toDeviceNdx = -1;

  for (int i = 0; i < m_devicesCount; i++)
  {
    if (strncmp(m_devices[i].topic, topic, strlen(m_devices[i].topic)) == 0)
    {
      toDeviceNdx = i;
      break;
    }
  }

  if (toDeviceNdx == -1)
  {

    Serial.println("=== INVALID DEVICE TO ROUTE ===");
  }

  int toDeviceId = toDeviceNdx + ROUTER_CAN_FIRST_DEVICE_ID;

  Serial.printf("route do device number %d, payload length is %d\n", toDeviceNdx, length);

  for (int i = 0; i < ROUTER_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + 10;
		m_queue[i].queueItemType = ROUTER_CAN_QIT_SEND_STRING_DATA;
		m_queue[i].deviceId = /*canMsgId_ROUTER_DEVICE*/ toDeviceId;

    m_queue[i].data = topic;
    m_queue[i].data.concat("\n");

    for (int pb = 0; pb < length; pb++)
      m_queue[i].data.concat((char)payload[pb]);

    m_queue[i].packetCmd = ROUTER_CAN_CMD_ROUTE_TO_DEVICE;
    m_queue[i].packetType = ROUTER_CAN_SPT_STRING2;
    m_queue[i].stringPacketBlockNumber = 0;
    m_queue[i].qState = qsDoIt;

		m_queueRequests++;

      Serial.println ("ROUTED MESSAGE:");
      Serial.println(m_queue[i].data);
      Serial.println ("--- END MESSAGE");

    Serial.println(" --> addToQueue");

		break;
	}
}



