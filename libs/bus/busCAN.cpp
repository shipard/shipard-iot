extern SHP_APP_CLASS *app;
extern int g_cntUarts;
void data2HexStr(uint8_t* data, size_t len, char str[]);

CAN_device_t CAN_cfg;

unsigned long previousMillis = 0;   // will store last time a CAN Message was send
const int interval = 10;          // interval at which send CAN Messages (milliseconds)
const int rx_queue_size = 1;       // Receive Queue size
int abcde = 10000000;

ShpBusCAN::ShpBusCAN() :
                            m_speed(-1),
                            m_mode(0),
                            m_rxPin(-1),
                            m_txPin(-1),
														m_queueRequests(0)
{
	for (int i = 0; i < BUS_CAN_CONTROL_QUEUE_LEN; i++)
	{
		m_queue[i].qState = qsFree;
		m_queue[i].startAfter = 0;
		m_queue[i].queueItemType = BUS_CAN_ITEM_TYPE_NONE;
		m_queue[i].bufferLen = 0;
	}
}

void Shp BusCAN::i nit(JsonVariant portCfg)
{
	/* serial port config format:
	 * --------------------------
	{
		"type": "bus/rs485",
		"portId": "rs485",
		"speed": 5,
		"mode": 0,
		"pinRX": 36,
		"pinTX": 4,
		"pinControl": 32
  }
	-----------------------------*/

	ShpIOPort::init(portCfg);

	// -- init members
	m_buffer[0] = 0;
	m_sbCnt = 0;
	m_bufNeedSend = 0;

	// -- rx / tx pin / control
	if (portCfg.containsKey("pinRX"))
		m_rxPin = portCfg["pinRX"];
	if (portCfg.containsKey("pinTX"))
		m_txPin = portCfg["pinTX"];

	if (m_txPin < 0 || m_rxPin < 0)
		return;

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_16;//(gpio_num_t)m_txPin;
  CAN_cfg.rx_pin_id = GPIO_NUM_17;//(gpio_num_t)m_rxPin;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

	Serial.printf("CAN START: rx: %d, tx: %d\n", m_rxPin, m_txPin);

	m_valueTopic = MQTT_TOPIC_THIS_DEVICE "/";
	m_valueTopic.concat(app->m_deviceId);
	m_valueTopic.concat("/");
	m_valueTopic.concat(m_portId);

  ESP32Can.CANInit();

  Serial.println("--CAN Initialized---");

	//m_hwSerial = new HardwareSerial(g_cntUarts++);
	//m_hwSerial->begin(m_speed, m_mode, m_rxPin, m_txPin);

	/*
	//delay(100);

	byte cmd[] = {
		0x01, 0x03,

		0xEC, 0x48,
		0x00, 0x01,
		0x00, 0x00 // CRC
	};

	uint16_t crc = modbusCalcCRC(cmd, 6);
	cmd[6] = crc >> 8;
	cmd[7] = crc & 0x00ff;

	addQueueItemWrite(8, cmd, 10000);
	*/
}

void ShpBusCAN::addQueueItemWrite(int8_t bufferLen, const byte* buffer, unsigned long startAfter)
{
	for (int i = 0; i < BUS_CAN_CONTROL_QUEUE_LEN; i++)
	{
		if (m_queue[i].qState != qsFree)
			continue;

		m_queue[i].qState = qsLocked;
		m_queue[i].startAfter = millis() + startAfter;
		m_queue[i].queueItemType = BUS_RS485_ITEM_TYPE_WRITE_BUFFER;
		m_queue[i].bufferLen = bufferLen;
		for (int bp = 0; bp < bufferLen; bp++)
			m_queue[i].buffer[bp] = buffer[bp];

		m_queue[i].qState = qsDoIt;
		m_queueRequests++;

		break;
	}
}

void ShpBusCAN::runQueueItem(int i)
{
	m_queueRequests--;

	if (m_queue[i].queueItemType == BUS_RS485_ITEM_TYPE_WRITE_BUFFER)
	{
		Serial.println("--CAN write---");
		//delay(30);

		//m_hwSerial->write(m_queue[i].buffer, m_queue[i].bufferLen);
		//m_hwSerial->flush();

		//delay(10);
		delay(50);
	}

	m_queue[i].qState = qsFree;
}


void ShpBusCAN::loop()
{

  //Serial.printf("CAN LOOP: %d\n", abcde++);

   CAN_frame_t rx_frame;

  unsigned long currentMillis = millis();

  // Receive next CAN frame from queue

  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {

    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      printf("New standard frame");
    }
    else {
      printf("New extended frame");
    }

    if (rx_frame.FIR.B.RTR == CAN_RTR) {
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
    }
    else {
      printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID,  rx_frame.FIR.B.DLC);
      for (int i = 0; i < rx_frame.FIR.B.DLC; i++) {
        printf("0x%02X ", rx_frame.data.u8[i]);
      }
      printf("\n");
    }
  }

  // Send CAN Message

  if (currentMillis - previousMillis >= interval) {

    abcde++;
    char buf[10];
    sprintf(buf, "%d", abcde);

    previousMillis = currentMillis;
      rx_frame.FIR.B.FF = CAN_frame_std;
      rx_frame.MsgID = 1;
      rx_frame.FIR.B.DLC = 8;
      rx_frame.data.u8[0] = buf[0];
      rx_frame.data.u8[1] = buf[1];
      rx_frame.data.u8[2] = buf[2];
      rx_frame.data.u8[3] = buf[3];
      rx_frame.data.u8[4] = buf[4];
      rx_frame.data.u8[5] = buf[5];
      rx_frame.data.u8[6] = buf[6];
      rx_frame.data.u8[7] = buf[7];
/*      rx_frame.data.u8[0] = 'h';
      rx_frame.data.u8[1] = 'e';
      rx_frame.data.u8[2] = 'l';
      rx_frame.data.u8[3] = 'l';
      rx_frame.data.u8[4] = 'o';
      rx_frame.data.u8[5] = 'c';
      rx_frame.data.u8[6] = 'a';
      rx_frame.data.u8[7] = 'n';*/

    Serial.printf("CAN SEND\n");
    ESP32Can.CANWriteFrame(&rx_frame);
    Serial.printf("CAN SEND done\n");
  }

	ShpIOPort::loop();
}

