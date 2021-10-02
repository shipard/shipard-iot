#ifndef SHP_BUS_I2C_H
#define SHP_BUS_I2C_H


class ShpBusI2C : public ShpIOPort 
{
	public:
		
		ShpBusI2C();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		void write2B(int address, uint8_t data1, uint8_t data2);
		void write16(int address, uint16_t data);

		TwoWire* wire() {return m_wire;}

	protected:

		void scan();

	private:
		
		int8_t m_pinSDA;
		int8_t m_pinSCL;

		TwoWire *m_wire;
		String m_devices;

};


#endif
