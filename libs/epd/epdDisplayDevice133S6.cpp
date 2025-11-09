extern SHP_APP_CLASS *app;



// *********
// SPI part
// *********

#include "esp_system.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define SPI_CS0		10
#define SPI_CS1		11
#define SPI_CLK		12
#define SPI_Data0	5
#define SPI_Data1	6
#define SPI_Data2	7
#define SPI_Data3	8
#define EPD_BUSY	15
#define EPD_RST		16
#define LOAD_SW		2

#define GPIO_LOW	0
#define GPIO_HIGH	1


#define SPI	SPI3_HOST
#define SPI_MAX_BUFFER_SIZE	32768

spi_device_handle_t spi;

esp_err_t initialSpi(void)
{
	esp_err_t status;

    spi_bus_config_t spiBusConfig={
        .data0_io_num=SPI_Data0,
        .data1_io_num=SPI_Data1,
        .sclk_io_num=SPI_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
				.data4_io_num=-1,
				.data5_io_num=-1,
				.data6_io_num=-1,
				.data7_io_num=-1,
        .max_transfer_sz = SPI_MAX_BUFFER_SIZE,
        .flags=SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t spiDeviceConfig = {
        .command_bits   = 8,
				.address_bits   = 0,
				.dummy_bits     = 0,
				.mode           = 0,                                //SPI mode 0
        .duty_cycle_pos=128,                     //50% duty cycle
				.cs_ena_pretrans=0,
        .cs_ena_posttrans=3,                     //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = SPI_MASTER_FREQ_10M,   //Clock out at 10 MHz
        .queue_size=7,                           //We want to be able to queue 7 transactions at a time
    };

    status=spi_bus_initialize(SPI, &spiBusConfig, SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(status);
    //Attach the LCD to the SPI bus
	status=spi_bus_add_device(SPI, &spiDeviceConfig, &spi);
    ESP_ERROR_CHECK(status);

#if SHOW_LOG
    if(status == ESP_OK)
    {
        printf("initialSpi() has been executed. \r\n");
    }
#endif

	return status;

}

void initialGpio(void)
{
	esp_err_t status;

    gpio_config_t ioConfig = {};

    ioConfig.pin_bit_mask = ((1ULL<<EPD_RST) | (1ULL<<SPI_CS0) | (1ULL<<SPI_CS1)
                            | (1ULL<<LOAD_SW)  );
    ioConfig.mode = GPIO_MODE_OUTPUT;
    ioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    status = gpio_config(&ioConfig);

#if SHOW_LOG
    if(status == ESP_OK)
    {
        printf("gpio_config(&ioConfig) has been executed. \r\n");
    }
#endif

    ioConfig.pin_bit_mask = (1ULL<<EPD_BUSY);
    ioConfig.intr_type = GPIO_INTR_NEGEDGE;
    ioConfig.mode = GPIO_MODE_INPUT;
    ioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    status = gpio_config(&ioConfig);

#if SHOW_LOG
    if(status == ESP_OK)
    {
    	printf("initialGpio() has been executed. \r\n");
    }
#endif

}

void delayms(unsigned int delayTime)
{
	vTaskDelay(delayTime/portTICK_PERIOD_MS);
}

esp_err_t spiTransmitCommand(unsigned char commandBuf)
{
	esp_err_t status;
	spi_transaction_t trans;

	memset(&trans, 0, sizeof(trans));
    trans.cmd = commandBuf;

    trans.length = 0;
    trans.tx_buffer = NULL;
    status = spi_device_transmit(spi, &trans);
    assert(status==ESP_OK);

	return status;

}

esp_err_t spiTransmitData(const unsigned char *dataBuffer, unsigned long dataLength)
{
	esp_err_t status=0;

	spi_transaction_ext_t trans_ext;

	while(dataLength >= SPI_MAX_BUFFER_SIZE)
	{
		memset(&trans_ext, 0, sizeof(trans_ext));
		trans_ext.command_bits = 0;
		trans_ext.base.length = SPI_MAX_BUFFER_SIZE * 8;
		// The trans_ext.base.length unit is bit, so the SPI_MAX_BUFFER_SIZE must be multiplied by 8.
		trans_ext.base.tx_buffer =  dataBuffer;
		trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
		status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);
		dataLength -= SPI_MAX_BUFFER_SIZE;
		dataBuffer += SPI_MAX_BUFFER_SIZE;
	}

	if(dataLength > 0)
	{
		memset(&trans_ext, 0, sizeof(trans_ext));
		trans_ext.command_bits = 0;
		trans_ext.base.length = dataLength * 8;
		// The trans_ext.base.length unit is bit, so the dataLength must be multiplied by 8.
		trans_ext.base.tx_buffer =  dataBuffer;
		trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
		status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);
	}

	return status;

}

esp_err_t spiReceiveData(unsigned char *dataBuffer, unsigned long dataLength)
{
	esp_err_t status=0;

	spi_transaction_ext_t trans_ext;

	while(dataLength > SPI_MAX_BUFFER_SIZE)
	{
		memset(&trans_ext, 0, sizeof(trans_ext));

		trans_ext.command_bits = 0;
		trans_ext.base.length = SPI_MAX_BUFFER_SIZE * 8;
		// The trans_ext.base.length unit is bit, so the SPI_MAX_BUFFER_SIZE must be multiplied by 8.
		trans_ext.base.rx_buffer =  dataBuffer;
		trans_ext.base.rxlength = dataLength * 8;
		trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
		status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);
		dataLength -= SPI_MAX_BUFFER_SIZE;
		dataBuffer += SPI_MAX_BUFFER_SIZE;
	}

	if(dataLength > 0)
	{
		memset(&trans_ext, 0, sizeof(trans_ext));

		trans_ext.command_bits = 0;
		trans_ext.base.length = dataLength * 8;
		// The trans_ext.base.length unit is bit, so the dataLength must be multiplied by 8.
		trans_ext.base.rx_buffer =  dataBuffer;
		trans_ext.base.rxlength = dataLength * 8;
		trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
		status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);
	}

	return status;

}

esp_err_t spiTransmitLargeData(unsigned char commandBuf, const unsigned char *dataBuffer, unsigned long dataLength)
{
	esp_err_t status=0;
	spi_transaction_t trans;
	spi_transaction_ext_t trans_ext;

	unsigned char firstPacket = 1;

	while(dataLength > SPI_MAX_BUFFER_SIZE)
	{
		if(firstPacket)
		{
			memset(&trans, 0, sizeof(trans));
			trans.cmd = commandBuf;
			trans.length = SPI_MAX_BUFFER_SIZE * 8;
			// The trans.length unit is bit, so the SPI_MAX_BUFFER_SIZE must be multiplied by 8.
			trans.tx_buffer = dataBuffer;
			trans.rx_buffer = NULL;
			status = spi_device_transmit(spi, &trans);
			firstPacket = 0;
		}
		else
		{
			memset(&trans_ext, 0, sizeof(trans_ext));
			trans_ext.command_bits = 0;
			trans_ext.base.length = SPI_MAX_BUFFER_SIZE * 8;
			// The trans_ext.base.length unit is bit, so the dataLength must be multiplied by 8.
			trans_ext.base.tx_buffer =  dataBuffer;
			trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
			status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);
		}

		dataLength -= SPI_MAX_BUFFER_SIZE;
		dataBuffer += SPI_MAX_BUFFER_SIZE;

	}

	if(dataLength > 0)
	{
		if(firstPacket)
		{
			memset(&trans, 0, sizeof(trans));
			trans.cmd = commandBuf;
			trans.length = dataLength * 8;
			// The trans.length unit is bit, so the dataLength must be multiplied by 8.
			trans.tx_buffer = dataBuffer;
			trans.rx_buffer = NULL;
			status = spi_device_transmit(spi, &trans);
			firstPacket = 0;
		}
		else
		{
			memset(&trans_ext, 0, sizeof(trans_ext));
			trans_ext.command_bits = 0;
			trans_ext.base.length = dataLength * 8;
			// The trans_ext.base.length unit is bit, so the dataLength must be multiplied by 8.
			trans_ext.base.tx_buffer =  dataBuffer;
			trans_ext.base.flags   = SPI_TRANS_VARIABLE_CMD;
			status = spi_device_transmit(spi, (spi_transaction_t*)&trans_ext);

		}
	}

	return status;

}

esp_err_t spiTransmit(unsigned char commandBuf, const unsigned char *dataBuffer, unsigned int dataLength)
{
	esp_err_t status=0;
	spi_transaction_t trans;

	memset(&trans, 0, sizeof(trans));

	if(dataLength < SPI_MAX_BUFFER_SIZE)
	{
		trans.cmd = commandBuf;
		trans.length = dataLength * 8;
		// The trans.length unit is bit, so the dataLength must be multiplied by 8.
		trans.tx_buffer = dataBuffer;
		trans.rx_buffer = NULL;
		status = spi_device_transmit(spi, &trans);

	}
	else status = -1; // The dataLength is over the SPI_MAX_BUFFER_SIZE

	return status;
}

esp_err_t spiReceive(unsigned char commandBuf, unsigned char *dataBuffer, unsigned int dataLength)
{
	esp_err_t status=0;

	spi_transaction_t trans;

	memset(&trans, 0, sizeof(trans));

	if(dataLength < SPI_MAX_BUFFER_SIZE)
	{
	    trans.cmd = commandBuf;
		trans.length= dataLength * 8;
		// The trans.length unit is bit, so the dataLength must be multiplied by 8.
		trans.rxlength = dataLength * 8;
		// The trans.rxlength unit is bit, so the dataLength must be multiplied by 8.
		trans.rx_buffer = dataBuffer;

		//==== SPI Transmit Command & Receive Data ====
		status = spi_device_transmit(spi, &trans);
		assert( status == ESP_OK );
	}
	else status = -1; // The dataLength is over the SPI_MAX_BUFFER_SIZE

	return status;

}

void setGpioLevel(unsigned char pinNumber, unsigned char voltageLevel)
{
	//==== Set GPIO voltage level ====
	gpio_set_level((gpio_num_t)pinNumber, voltageLevel);
}

unsigned char getGpioLevel(unsigned char pinNumber)
{
	unsigned char voltageLevel;
	//==== Get GPIO voltage level ====
	voltageLevel = gpio_get_level((gpio_num_t)pinNumber);

	return voltageLevel;
}



// ***************
// EPD part
// ***************

#define ERROR 1
#define DONE 0

#define SHOW_LOG 1


#define BLACK         0x00
#define WHITE         0x11
#define YELLOW        0x22
#define RED           0x33
#define BLUE          0x55
#define GREEN         0x66


#define PSR             0x00
#define PWR             0x01
#define POF             0x02
#define PON             0x04
#define BTST_N          0x05
#define BTST_P          0x06
#define DTM             0x10
#define DRF             0x12
#define CDI             0x50
#define TCON            0x60
#define TRES            0x61
#define PTLW            0x83
#define AN_TM           0x74
#define AGID            0x86
#define BUCK_BOOST_VDDN 0xB0
#define TFT_VCOM_POWER  0xB1
#define EN_BUF          0xB6
#define BOOST_VDDP_EN   0xB7
#define CCSET           0xE0
#define PWS             0xE3
#define CMD66           0xF0

#define FIRST_DATA_PACKET	1
#define NOT_FIRST_DATA_PACKET	0

#define PTLW_ENABLE  0x01
#define PTLW_DISABLE 0x00



const unsigned char spiCsPin[2] = {
		SPI_CS0, SPI_CS1
};
const unsigned char PSR_V[2] = {
	0xDF, 0x69
};
const unsigned char PWR_V[6] = {
	0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38
};
const unsigned char POF_V[1] = {
	0x00
};
const unsigned char DRF_V[1] = {
	0x01
};
const unsigned char CDI_V[1] = {
	0xF7
};
const unsigned char TCON_V[2] = {
	0x03, 0x03
};
const unsigned char TRES_V[4] = {
	0x04, 0xB0, 0x03, 0x20
};
const unsigned char CMD66_V[6] = {
	0x49, 0x55, 0x13, 0x5D, 0x05, 0x10
};
const unsigned char EN_BUF_V[1] = {
	0x07
};
const unsigned char CCSET_V[1] = {
	0x01
};
const unsigned char PWS_V[1] = {
	0x22
};
const unsigned char AN_TM_V[9] = {
	0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55
};

const unsigned char AGID_V[1] = {
	0x10
};

const unsigned char BTST_P_V[2] = {
	0xE8, 0x28
};
const unsigned char BOOST_VDDP_EN_V[1] = {
	0x01
};
const unsigned char BTST_N_V[2] = {
	0xE8, 0x28
};
const unsigned char BUCK_BOOST_VDDN_V[1] = {
	0x01
};
const unsigned char TFT_VCOM_POWER_V[1] = {
	0x02
};

char partialWindowUpdateStatus = DONE;

//================== GPIO Setting ====================================
void resetPin(unsigned int pinStatus)
{
	setGpioLevel(EPD_RST, pinStatus);
}

void setPinCsAll(unsigned int setLevel){
	unsigned char i;
	for(i=0;i<2;i++)
	{
		setGpioLevel(spiCsPin[i], setLevel);
	}
}

void setPinCs(unsigned char csNumber, unsigned int setLevel){
	setGpioLevel(spiCsPin[csNumber], setLevel);
}
void checkBusyHigh(void)// If BUSYN=0 then waiting
{
	Serial.println("checkBusyHigh WAIT");
	while(!(getGpioLevel(EPD_BUSY)));
	Serial.println("checkBusyHigh DONE");
}

void checkBusyLow(void)// If BUSYN=1 then waiting
{
	while(getGpioLevel(EPD_BUSY));
}
//====================================================================

void epdHardwareReset(void)
{
	resetPin(GPIO_HIGH);
	delayms(30);

	resetPin(GPIO_LOW);
	delayms(30);
	resetPin(GPIO_HIGH);
	delayms(30);
}

void writeEpd(unsigned char epdCommand, const unsigned char *epdData, unsigned int epdDataLength)
{
	spiTransmit(epdCommand, epdData, epdDataLength);
}

void readEpd(unsigned char epdCommand, unsigned char *epdData, unsigned int epdDataLength)
{
	spiReceive(epdCommand, epdData, epdDataLength);
}

void writeEpdCommand(unsigned char epdCommand)
{
	spiTransmitCommand(epdCommand);
}

void writeEpdData(const unsigned char *epdData, unsigned int epdDataLength)
{
	spiTransmitData(epdData, epdDataLength);
}

void initEPD(void)
{
	epdHardwareReset();
	checkBusyHigh();
	//checkBusyLow();

	setPinCs(0,GPIO_LOW);
	writeEpd(AN_TM, AN_TM_V, sizeof(AN_TM_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(CMD66, CMD66_V, sizeof(CMD66_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(PSR, PSR_V, sizeof(PSR_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(CDI, CDI_V, sizeof(CDI_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(TCON, TCON_V, sizeof(TCON_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(AGID, AGID_V, sizeof(AGID_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(PWS, PWS_V, sizeof(PWS_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(CCSET, CCSET_V, sizeof(CCSET_V));
	setPinCsAll(GPIO_HIGH);

	setPinCsAll(GPIO_LOW);
	writeEpd(TRES, TRES_V, sizeof(TRES_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(PWR, PWR_V, sizeof(PWR_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(BTST_P, BTST_P_V, sizeof(BTST_P_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(BTST_N, BTST_N_V, sizeof(BTST_N_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
	setPinCsAll(GPIO_HIGH);

	setPinCs(0,GPIO_LOW);
	writeEpd(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
	setPinCsAll(GPIO_HIGH);

#if SHOW_LOG
    printf("initEPD() has been executed. \r\n");
#endif
}

unsigned char checkDriverICStatus(void)
{
	unsigned char csx, status = DONE;
	unsigned char dataBuf[3];

	for(csx=0 ; csx < 2 ; csx++)
	{
		memset(dataBuf, 0, sizeof(dataBuf));
		setPinCs(csx,GPIO_LOW);
		readEpd(0xF2, dataBuf, sizeof(dataBuf));
		setPinCs(csx,GPIO_HIGH);
#if SHOW_LOG
		printf("Driver IC [%d] = 0x%02X 0x%02X 0x%02X \r\n", csx, dataBuf[0], dataBuf[1], dataBuf[2]);
#endif
		if((dataBuf[0] & 0x01) == 0x01)
		{
#if SHOW_LOG
			printf("Driver IC [%d] is ready. \r\n",csx);
#endif
		}
		else
		{
#if SHOW_LOG
			printf("Driver IC [%d] did not reply. \r\n",csx);
#endif
			status = ERROR;
		}

	}

	return status;
}

void epdDisplay(void)
{

#if SHOW_LOG
    printf("Write PON \r\n");
#endif
	setPinCsAll(GPIO_LOW);
	writeEpdCommand(PON);
	checkBusyHigh();
	setPinCsAll(GPIO_HIGH);

#if SHOW_LOG
	printf("Write DRF \r\n");
#endif
	setPinCsAll(GPIO_LOW);
	delayms(30);
	writeEpd(DRF, DRF_V, sizeof(DRF_V));
	checkBusyHigh();
	setPinCsAll(GPIO_HIGH);

#if SHOW_LOG
	printf("Write POF \r\n");
#endif
	setPinCsAll(GPIO_LOW);
	writeEpd(POF, POF_V, sizeof(POF_V));
	checkBusyHigh();
	setPinCsAll(GPIO_HIGH);
#if SHOW_LOG
	printf("Display Done!! \r\n");
#endif
}



// Display screen parameters (already provided)
#define EPD_WIDTH 1200         // Total display width (pixels)
#define EPD_HEIGHT 1600        // Display height (pixels)
#define FIRST_PACK_SIZE 480000 // First data packet size (bytes)
#define TOTAL_IMAGE_SIZE 960000 // Total image data size (bytes)




char partialWindowUpdateWithImageData(unsigned char csx, unsigned char const *imageData, unsigned long imageDataLength,
     unsigned int xStart, unsigned int yStart, unsigned int xPixel, unsigned int yLine, unsigned char epdDisplayEnable)
{
	unsigned char status = DONE;
	unsigned int HRST, HRED, VRST, VRED;
	unsigned char partialWindowData[9];

	HRST = xStart * 2;
	HRED = (xStart + xPixel) * 2 - 1; // The range is 0 ~ 1199
	VRST = yStart / 2;
	VRED = (yStart + yLine) / 2 - 1; // The range is 0 ~ 799

#if SHOW_LOG
	printf("csx = %d ; HRST = %d ; HRED = %d ; VRST = %d ; VRED = %d \r\n",csx, HRST, HRED, VRST, VRED);
#endif

	// HRST[10:0] = 8n (n = 0,1,2…)
	if (HRST % 8 != 0){
		status = -1;
#if SHOW_LOG
		printf("status = -1 ; There is a problem with xStart. \r\n");
#endif
	}
	// HRED[10:0] = 8m+3 (m = 4,5,6…)
	else if ((HRED - 7) % 8 != 0) {
		status = -2;
#if SHOW_LOG
		printf("status = -2 ; There is a problem with xPixel. \r\n");
#endif
	}
	//  xStart <= 584 ; xPixel <= 600
	else if ((xStart > 584) | (xPixel > 600)) {
		status = -3;
#if SHOW_LOG
		printf("status = -3 ; xStart or xPixel is over range. \r\n");
#endif
	}
	// HRED - HRST + 1 >= 32 & HRED + 1 <= 1200
	else if ((HRED - HRST + 1 < 32) | (HRED + 1 > 1200)){
		status = -4;
#if SHOW_LOG
		printf("status = -4 ; There is a problem with xStart & xPixel. \r\n");
#endif
	}
	else if ((yStart + yLine) % 2 != 0){
		status = -5;
#if SHOW_LOG
		printf("status = -5 ; yStart + yLine must be an even number. \r\n");
#endif
	}
	// yStart <= 1596 ; yLine <= 1600
	else if ((yStart > 1596) | (yLine > 1600)) {
		status = -6;
#if SHOW_LOG
		printf("status = -6 ; yStart or yLine is over range. \r\n");
#endif
	}
	//VRST - VRED + 1 > 0 & VRED + 1 <= 800
	else if (((int)(VRED - VRST) + 1 <= 0) | (VRED + 1 > 800)){
		status = -7;
#if SHOW_LOG
		printf("status = -7 ; There is a problem with yStart & yLine. \r\n");
#endif
	}
	else if(csx > 1){
		status = -8;
#if SHOW_LOG
		printf("status = -8 ; There is a problem with cxs. \r\n");
#endif
	}
	else
	{
		memset(partialWindowData,0,sizeof(partialWindowData));
		partialWindowData[0] = (unsigned char)(HRST >> 8);
		partialWindowData[1] = (unsigned char)(HRST);
		partialWindowData[2] = (unsigned char)(HRED >> 8);
		partialWindowData[3] = (unsigned char)(HRED);
		partialWindowData[4] = (unsigned char)(VRST >> 8);
		partialWindowData[5] = (unsigned char)(VRST);
		partialWindowData[6] = (unsigned char)(VRED >> 8);
		partialWindowData[7] = (unsigned char)(VRED);
		partialWindowData[8] = PTLW_ENABLE;

		setPinCs(csx,GPIO_LOW);
		writeEpd(CMD66, CMD66_V, sizeof(CMD66_V));
		setPinCs(csx,GPIO_HIGH);

		setPinCs(csx,GPIO_LOW);
		writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
		setPinCs(csx,GPIO_HIGH);

		setPinCs(csx,GPIO_LOW);
		spiTransmitLargeData(DTM, imageData, imageDataLength);
		setPinCs(csx,GPIO_HIGH);

	}

	if(status != DONE)
	{
		partialWindowUpdateStatus = ERROR;
#if SHOW_LOG
		printf("partialWindowUpdateStatus = ERROR \r\n");
#endif
	}

	if(epdDisplayEnable)
	{
		if(partialWindowUpdateStatus == DONE) epdDisplay();

		delayms(300);

		//========================= Turn off PTLW =========================
		memset(partialWindowData,0,sizeof(partialWindowData));
		partialWindowData[8] = PTLW_DISABLE;
		partialWindowUpdateStatus = DONE;

		setPinCsAll(GPIO_LOW);
		writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
		setPinCsAll(GPIO_HIGH);
		//=================================================================
	}

	return status;
}

char  partialWindowUpdateWithoutImageData(unsigned char csx, unsigned int xStart, unsigned int yStart,
	 unsigned int xPixel, unsigned int yLine, unsigned char epdDisplayEnable)
{
	unsigned char status = DONE;
	unsigned int HRST, HRED, VRST, VRED;
	unsigned char partialWindowData[9];

	HRST = xStart * 2;
	HRED = (xStart + xPixel) * 2 - 1; // The range is 0 ~ 1199
	VRST = yStart / 2;
	VRED = (yStart + yLine) / 2 - 1; // The range is 0 ~ 799

#if SHOW_LOG
	printf("csx = %d ; HRST = %d ; HRED = %d ; VRST = %d ; VRED = %d \r\n",csx, HRST, HRED, VRST, VRED);
#endif

	// HRST[10:0] = 8n (n = 0,1,2…)
	if (HRST % 8 != 0){
		status = -1;
#if SHOW_LOG
		printf("status = -1 ; There is a problem with xStart. \r\n");
#endif
	}
	// HRED[10:0] = 8m+3 (m = 4,5,6…)
	else if ((HRED - 7) % 8 != 0) {
		status = -2;
#if SHOW_LOG
		printf("status = -2 ; There is a problem with xPixel. \r\n");
#endif
	}
	//  xStart <= 584 ; xPixel <= 600
	else if ((xStart > 584) | (xPixel > 600)) {
		status = -3;
#if SHOW_LOG
		printf("status = -3 ; xStart or xPixel is over range. \r\n");
#endif
	}
	// HRED - HRST + 1 >= 32 & HRED + 1 <= 1200
	else if ((HRED - HRST + 1 < 32) | (HRED + 1 > 1200)){
		status = -4;
#if SHOW_LOG
		printf("status = -4 ; There is a problem with xStart & xPixel. \r\n");
#endif
	}
	else if ((yStart + yLine) % 2 != 0){
		status = -5;
#if SHOW_LOG
		printf("status = -5 ; yStart + yLine must be an even number. \r\n");
#endif
	}
	// yStart <= 1596 ; yLine <= 1600
	else if ((yStart > 1596) | (yLine > 1600)) {
		status = -6;
#if SHOW_LOG
		printf("status = -6 ; yStart or yLine is over range. \r\n");
#endif
	}
	//VRST - VRED + 1 > 0 & VRED + 1 <= 800
	else if (((int)(VRED - VRST) + 1 <= 0) | (VRED + 1 > 800)){
		status = -7;
#if SHOW_LOG
		printf("status = -7 ; There is a problem with yStart & yLine. \r\n");
#endif
	}
	else if(csx > 1){
		status = -8;
#if SHOW_LOG
		printf("status = -8 ; There is a problem with cxs. \r\n");
#endif
	}
	else
	{
		memset(partialWindowData,0,sizeof(partialWindowData));
		partialWindowData[0] = (unsigned char)(HRST >> 8);
		partialWindowData[1] = (unsigned char)(HRST);
		partialWindowData[2] = (unsigned char)(HRED >> 8);
		partialWindowData[3] = (unsigned char)(HRED);
		partialWindowData[4] = (unsigned char)(VRST >> 8);
		partialWindowData[5] = (unsigned char)(VRST);
		partialWindowData[6] = (unsigned char)(VRED >> 8);
		partialWindowData[7] = (unsigned char)(VRED);
		partialWindowData[8] = PTLW_ENABLE;

		setPinCs(csx,GPIO_LOW);
		writeEpd(CMD66, CMD66_V, sizeof(CMD66_V));
		setPinCs(csx,GPIO_HIGH);

		setPinCs(csx,GPIO_LOW);
		writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
		setPinCs(csx,GPIO_HIGH);
	}

	if(status != DONE)
	{
		partialWindowUpdateStatus = ERROR;
#if SHOW_LOG
		printf("partialWindowUpdateStatus = ERROR \r\n");
#endif
	}

	if(epdDisplayEnable)
	{
		if(partialWindowUpdateStatus == DONE) epdDisplay();

		delayms(300);

		//========================= Turn off PTLW =========================
		memset(partialWindowData,0,sizeof(partialWindowData));
		partialWindowData[8] = PTLW_DISABLE;
		partialWindowUpdateStatus = DONE;

		setPinCsAll(GPIO_LOW);
		writeEpd(PTLW, partialWindowData, sizeof(partialWindowData));
		setPinCsAll(GPIO_HIGH);
		//=================================================================
	}

	return status;
}


// ***************
// ShpEpdDisplayDevice133S6 part
// ***************

/*
#define EPD_13IN3E_BLACK        0x0
#define EPD_13IN3E_WHITE        0x1
#define EPD_13IN3E_YELLOW       0x2
#define EPD_13IN3E_RED          0x3
#define EPD_13IN3E_BLUE         0x5
#define EPD_13IN3E_GREEN        0x6

*/

#define EPD13S6_BLACK         0x0
#define EPD13S6_WHITE         0x1
#define EPD13S6_YELLOW        0x2
#define EPD13S6_RED           0x3
#define EPD13S6_BLUE          0x5
#define EPD13S6_GREEN         0x6
#define EPD13S6_ORANGE        0x7 // NOT SUPPORTED

#define epdColorMap {EPD13S6_BLACK,EPD13S6_WHITE,EPD13S6_RED,EPD13S6_YELLOW,EPD13S6_BLUE,EPD13S6_GREEN, EPD13S6_BLACK, EPD13S6_ORANGE}

#define SHP_BF_COUNT_BITS             5
#define SHP_BF_COUNT_MASK             0b00011111

//#define EPD_WIDTH 1200         // Total display width (pixels)
//#define EPD_HEIGHT 1600        // Display height (pixels)
#define HALF_WIDTH 300


ShpEpdDisplayDevice133S6::ShpEpdDisplayDevice133S6(ShpEpdDisplay *epdDisplay) : ShpEpdDisplayDeviceCore(epdDisplay)
{
}


void ShpEpdDisplayDevice133S6::initDevice()
{
  Serial.println("initEPD DEVICE");
	initialGpio();
	initialSpi();
  epdHardwareReset();

  //checkDriverICStatus();

	setPinCsAll(GPIO_HIGH);
	//===============================================================

	//epdStatus = checkDriverICStatus();
	Serial.println("initEPD 1");

  initEPD();
}

void ShpEpdDisplayDevice133S6::setRotation(uint8_t rotation)
{
  //display.setRotation(rotation);
}

void ShpEpdDisplayDevice133S6::displayImage()
{
  showBitmap_PSRAM();
}


void ShpEpdDisplayDevice133S6::showBitmap_PSRAM()
{
  uint8_t colorMap[] = epdColorMap;
  unsigned char pixBuff [EPD_WIDTH];

  // -- FIRST half
  setPinCsAll(GPIO_HIGH);        // Deselect all
  setPinCs(0, 0);                // Select the first section (main display)
  writeEpdCommand(DTM);          // Send data transfer mode command

  uint16_t displayPosX = 0;
  uint16_t displayPosY = 0;
  uint16_t displayPosXHalf = 0;

  setPinCsAll(GPIO_HIGH);        // Deselect all
  setPinCs(0, 0);                // Select the first section (main display)
  writeEpdCommand(DTM);          // Send data transfer mode command

  for (size_t pos = 64; pos < m_epdDisplay->m_imgDataSize; pos++)
  {
    uint8_t count = m_epdDisplay->m_imgData[pos] & SHP_BF_COUNT_MASK;
    uint8_t pixel_color = m_epdDisplay->m_imgData[pos] >> SHP_BF_COUNT_BITS;
    uint8_t color = colorMap[pixel_color];
    for (uint8_t xx = 0; xx < count; xx++)
    { // num[new_index] = (color1 << 4) | color2;
      if (displayPosX % 2 == 0)
        pixBuff[displayPosXHalf] = color << 4;
      else
      {
        pixBuff[displayPosXHalf] |= color;
        displayPosXHalf++;
      }

      displayPosX++;
      if (displayPosX == 1200)
      {
        writeEpdData(pixBuff, HALF_WIDTH); // Send the first half of each row's data
        vTaskDelay(pdMS_TO_TICKS(1));          // Delay 1ms to avoid hardware overload
        displayPosY++;
        displayPosX = 0;
        displayPosXHalf = 0;
      }
    }
  }
  setPinCsAll(GPIO_HIGH);        // Deselect


  // -- SECOND half
  setPinCs(1, 0);                // Select the second section (secondary display)
  writeEpdCommand(DTM);          // Send data transfer mode command

  displayPosX = 0;
  displayPosY = 0;
  displayPosXHalf = 0;
  for (size_t pos = 64; pos < m_epdDisplay->m_imgDataSize; pos++)
  {
    uint8_t count = m_epdDisplay->m_imgData[pos] & SHP_BF_COUNT_MASK;
    uint8_t pixel_color = m_epdDisplay->m_imgData[pos] >> SHP_BF_COUNT_BITS;
    uint8_t color = colorMap[pixel_color];
    for (uint8_t xx = 0; xx < count; xx++)
    {
      if (displayPosX % 2 == 0)
        pixBuff[displayPosXHalf] = color << 4;
      else
      {
        pixBuff[displayPosXHalf] |= color;
        displayPosXHalf++;
      }
      displayPosX++;
      if (displayPosX == 1200)
      {
        writeEpdData(pixBuff + HALF_WIDTH, HALF_WIDTH); // Send the second half of each row's data
        vTaskDelay(pdMS_TO_TICKS(1));                   // Delay 1ms to avoid hardware overload
        displayPosY++;
        displayPosX = 0;
        displayPosXHalf = 0;
      }
    }
  }
  setPinCsAll(GPIO_HIGH);        // Deselect


  // -- Refresh the display
  epdDisplay();                  // Trigger display
  vTaskDelay(pdMS_TO_TICKS(10)); // Delay 10ms to ensure refresh completion
}
