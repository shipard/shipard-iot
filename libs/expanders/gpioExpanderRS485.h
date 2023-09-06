#ifndef SHP_GPIO_EXPANDER_RS485_H
#define SHP_GPIO_EXPANDER_RS485_H

#define SHP_GPIO_RS485_MODBUS_UNKNOWN 0
#define SHP_GPIO_RS485_MODBUS_REL8_NONAME 1
#define SHP_GPIO_RS485_MAX_VALUE 2

class ShpBusRS485;

class ShpGpioExpanderRS485 : public ShpGpioExpander
{
	public:

		ShpGpioExpanderRS485();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

		virtual void setPinState(uint8_t pin, uint8_t value);

	protected:

		uint16_t modbusCalcCRC(uint8_t *buffer, uint8_t bufferLen);

	private:

		uint8_t m_expType;
		const char *m_busPortId;
		ShpBusRS485 *m_bus;
		int m_address;
};


#endif
