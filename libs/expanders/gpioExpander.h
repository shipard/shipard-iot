#ifndef SHP_GPIO_EXPANDER_H
#define SHP_GPIO_EXPANDER_H

#define SHP_GPIO_EXP_MAX_PINS 16

class ShpInputBinary;


class ShpGpioExpander : public ShpIOPort 
{
	public:
		
		ShpGpioExpander();

		virtual void setPinState(uint8_t pin, uint8_t value);
		void setInputPinIOPort(uint8_t pin, ShpInputBinary *ioPort);

	protected:

		ShpInputBinary *m_inputIOPorts[SHP_GPIO_EXP_MAX_PINS];

};


#endif
