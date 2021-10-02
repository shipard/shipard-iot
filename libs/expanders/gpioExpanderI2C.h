#ifndef SHP_GPIO_EXPANDER_I2C_H
#define SHP_GPIO_EXPANDER_I2C_H

#define SHP_GPIO_CHIP_PCF8575 0
#define SHP_GPIO_CHIP_MCP23017 1
#define SHP_GPIO_OLIMEX_MODIO 2
#define SHP_GPIO_OLIMEX_MODIO2 3
#define SHP_GPIO_CHIP_MAX_VALUE 3

class ShpGpioExpanderI2C : public ShpGpioExpander 
{
	public:
		
		ShpGpioExpanderI2C();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();

		virtual void setPinState(uint8_t pin, uint8_t value);

	protected:

		void write(uint16_t data);
		void write2B(uint8_t data1, uint8_t data2);

	private:
		
		uint8_t m_expType;
		const char *m_busPortId;//[MAX_IO_PORT_ID_SIZE];
		ShpBusI2C *m_bus;
		int m_address;
		uint16_t m_bits;

};


#endif
